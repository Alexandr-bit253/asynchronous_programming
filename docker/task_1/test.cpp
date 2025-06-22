create_encrypted_function_rc4(
    M_VirtExecuter &virt_executer, // текущее состояние регистров
    uint32_t block_rva,            // rva блока данных для шифрования
    uint32_t block_size)           // size блока данных для шифрования
{
    bool is_need_add_rnd_cmds{false};
    uint32_t add_rnd_cmds_count{5};
    M_CodeCreator m_code{};
    M_BufferEx block_data{};
    uint32_t *block_ptr{};
    uint32_t rnd_add_value32{}, rnd_start_iv{}, rnd_start_iv0{};
    uint32_t first_cipher_dword{};
    uint64_t rnd_value_64{};
    uint32_t rnd_jmp_to{};
    size_t key_len{};
    uint32_t crc32{};

    // необходимые регистры
    uint8_t reg_x{};
    uint8_t reg_y{};
    uint8_t reg_s{};
    uint8_t reg_counter{};
    uint8_t reg_temp{};

    // инициализация используемых регистров
    reg_x = virt_executer.get_rnd_reg_32_64();
    if (reg_x == REG_ERR)
    {
        printf("[!!!!!] failed to acquire a random register (reg_x)\n");
        goto gen_err;
    }
    virt_executer.exclude_reg_32_64(reg_x);

    reg_y = virt_executer.get_rnd_reg_32_64();
    if (reg_y == REG_ERR)
    {
        printf("[!!!!!] failed to acquire a random register (reg_y)\n");
        goto gen_err;
    }
    virt_executer.exclude_reg_32_64(reg_y);

    reg_s = virt_executer.get_rnd_reg_32_64();
    if (reg_s == REG_ERR)
    {
        printf("[!!!!!] failed to acquire a random register (reg_s)\n");
        goto gen_err;
    }
    virt_executer.exclude_reg_32_64(reg_s);

    reg_counter = virt_executer.get_rnd_reg_32_64();
    if (reg_counter == REG_ERR)
    {
        printf("[!!!!!] failed to acquire a random register (reg_counter)\n");
        goto gen_err;
    }
    virt_executer.exclude_reg_32_64(reg_counter);

    reg_temp = virt_executer.get_rnd_reg_32_64();
    if (reg_temp == REG_ERR)
    {
        printf("[!!!!!] failed to acquire a random register (reg_temp)\n");
        goto gen_err;
    }
    virt_executer.exclude_reg_32_64(reg_temp);

    // размер шифруемого кода должен быть больше 4 байт и кратен 4
    block_size = block_size / 4;
    if (block_size == 0)
        return true;

    printf("START ENCRYPT BLOCK. Block RVA = 0x%08x, block SIZE = 0x%08x. Decryptor RVA = 0x%08x\n", block_rva, block_size * 4, virt_executer.rva_cur);
    printf("Used registers:\n"
           "- reg_x     = %s\n"
           "- reg_y     = %s\n"
           "- reg_s     = %s\n"
           "- reg_counter64 = %s\n"
           "- reg_temp = %s\n",
           regs_64_names_x64[reg_x], regs_64_names_x64[reg_y], regs_64_names_x64[reg_s], regs_64_names_x64[reg_counter], regs_64_names_x64[reg_temp]);

    //////////////////////////////////////////////////////////////////////////////////////
    // 1) ШИФРОВАНИЕ
    //////////////////////////////////////////////////////////////////////////////////////
    // выделяем память
    if (block_data.alloc_zero_buffer(block_size * 4) == false)
    {
        printf("[-] can't alloc memory for encrypt, SIZE = 0x%08x\n", block_size);
        goto gen_err;
    }
    block_ptr = reinterpret_cast<uint32_t *>(block_data.get_ptr());
    if (m_PESections.Read(block_rva, reinterpret_cast<uint8_t *>(block_ptr), block_size * 4) == false)
    {
        printf("[-] can't read data from PE file. RVA: 0x%08x, Size: 0x%08x\n", block_rva, block_size * 4);
        goto gen_err;
    }

    //////////////////////////////////////////////////////////////////////////////////////
    // вычисление crc_32 checksum
    //////////////////////////////////////////////////////////////////////////////////////
    if (calculate_crc_32_checksum(block_ptr, block_size, &crc32) == false)
    {
        printf("[!!!!!] failed to calculate crc 32 checksum before encrypt\n");
        goto gen_err;
    }

    // генерация случайной длинны ключа
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> len_dist(8, 256);
    key_len = len_dist(gen);

    // генерация случайного ключа
    unsigned char key[256];
    std::uniform_int_distribution<int> byte_dist(0, 255);
    for (size_t i = 0; i < key_len; ++i)
    {
        key[i] = static_cast<unsigned char>(byte_dist(gen));
    }
    // инициализация key
    rc4.SetKey(key, key_len);

    // шифрование данных
    rc4.Encrypt(block_ptr, block_size);

    // записываем зашифрованные данные
    m_PESections.Write(block_rva, block_ptr, block_size * 4);

    // восстанавливаем key в начальное состояние
    rc4.SetKey(key, key_len);

    // выделяем место под X, Y, state
    rc4.SetSymbols(symbols);

    //////////////////////////////////////////////////////////////////////////////////////
    // 2) ИЗМЕНЕНИЕ регистра для базы + случайный прышок
    //////////////////////////////////////////////////////////////////////////////////////

    // --- миграция значения базового регистра
    if (migrate_reg_base(virt_executer) == false)
        goto gen_err;

    // --- случайный прышок 30+rnd()%20 байт
    rnd_value_64 = m_rnd.gen_rnd_64();
    m_PESections.Write(virt_executer.rva_cur, &rnd_value_64, 8);

    rnd_value_64 = m_rnd.gen_rnd_64();
    m_PESections.Write(virt_executer.rva_cur + 8, &rnd_value_64, 8);

    rnd_jmp_to = virt_executer.rva_cur + 30 + m_rnd.gen_rnd_32() % 20;
    if (create_rnd_jmp(virt_executer, virt_executer.rva_cur, rnd_jmp_to) == false)
        goto gen_err;
    virt_executer.rva_cur = rnd_jmp_to;

    //////////////////////////////////////////////////////////////////////////////////////
    // 3) генерация кода ДЕШИФРОВЩИКА
    //////////////////////////////////////////////////////////////////////////////////////
    uint32_t label_rva_check_1, label_rva_loop;

    if (m_is_app_64)
    {
        // проверка нужно ли расшифровывать?
        // для этого проверяем случайные байты из нескольких участков
        first_cipher_dword = block_ptr[0];

        // cmp dword ptr [reg_base + block_rva + 0], first_cipher_dword
        if (m_code.arifmetic_rm64_imm32_dword_ptr(CMP, virt_executer.reg_base, 0, REG_ERR, block_rva, first_cipher_dword) == false)
            goto gen_err;
        save_code(m_code, virt_executer.rva_cur, true);

        label_rva_check_1 = virt_executer.rva_cur;

        // jnz
        if (m_code.jCC_32(JNZ, label_rva_check_1, 0xffffffff) == false)
            goto gen_err;
        save_code(m_code, virt_executer.rva_cur, true);

        // запись x и y в выделенную память
        if (!m_code.mov_rm64_imm32_uint32_t_ptr(virt_executer.reg_base, 0, REG_ERR, rc4.GetXRVA(), rc4.GetX()))
            goto gen_err;
        save_code(m_code, virt_executer.rva_cur, true);

        if (!m_code.mov_rm64_imm32_uint32_t_ptr(virt_executer.reg_base, 0, REG_ERR, rc4.GetYRVA(), rc4.GetY()))
            goto gen_err;
        save_code(m_code, virt_executer.rva_cur, true);

        // запись state в выделенную память
        const uint8_t *state_ptr = rc4.GetState();
        for (int i = 0; i < 256; i += 4)
        {
            uint32_t state_part = *reinterpret_cast<const uint32_t *>(state_ptr + i);
            m_code.mov_rm64_imm32_uint32_t_ptr(virt_executer.reg_base, 0, REG_ERR, rc4.GetStateRVA() + i, state_part);
            save_code(m_code, virt_executer.rva_cur, true);
        }

        // инициализация регистров rc4-дешифратора
        if (!m_code.mov_reg64_rm64(reg_x, virt_executer.reg_base, 0, REG_ERR, rc4.GetXRVA()))
            goto gen_err;
        save_code(m_code, virt_executer.rva_cur, true);

        if (!m_code.mov_reg64_rm64(reg_y, virt_executer.reg_base, 1, REG_ERR, rc4.GetXRVA()))
            goto gen_err;
        save_code(m_code, virt_executer.rva_cur, true);

        if (!m_code.lea_reg64_rm64(reg_s, virt_executer.reg_base, 0, REG_ERR, rc4.GetStateRVA()))
            goto gen_err;
        save_code(m_code, virt_executer.rva_cur, true);

        // mov reg_counter, 0
        if (!m_code.mov_reg64_imm64(reg_counter, 0))
            goto gen_err;
        save_code(m_code, virt_executer.rva_cur, true);

        label_rva_loop = virt_executer.rva_cur;

        // x = (x + 1)
        // add reg_x, 1
        if (!m_code.arifmetic_reg64_imm8s(ADD, reg_x, 1))
            goto gen_err;
        save_code(m_code, virt_executer.rva_cur, true);

        // and reg_x, 0xff
        if (!m_code.arifmetic_reg64_imm32(AND, reg_x, 0xFF))
            goto gen_err;
        save_code(m_code, virt_executer.rva_cur, true);

        // mov reg_temp, state[x]
        if (!m_code.mov_reg8_rm8(reg_temp, reg_s, 1, reg_x, 0))
            goto gen_err;
        save_code(m_code, virt_executer.rva_cur, true);

        // y = (state[x] + y) % 256
        // add reg_y, reg_temp
        if (!m_code.arifmetic_reg64_reg64(ADD, reg_y, reg_temp))
            goto gen_err;
        save_code(m_code, virt_executer.rva_cur, true);

        // and reg_y, 0xff
        if (!m_code.arifmetic_reg64_imm32(AND, reg_y, 0xFF))
            goto gen_err;
        save_code(m_code, virt_executer.rva_cur, true);

        // swap state[x] state[y]
        if (!m_code.xchg_rm8_reg8(reg_temp, reg_s, 1, reg_y, 0))
            goto gen_err;
        save_code(m_code, virt_executer.rva_cur, true);

        if (!m_code.mov_rm8_reg8(reg_temp, reg_s, 0, reg_x, 0))
            goto gen_err;
        save_code(m_code, virt_executer.rva_cur, true);

        // xorIndex = (state[x] + state[y]) % 256;
        if (!m_code.arifmetic_reg8_rm8_mode64(ADD, reg_temp, reg_s, 1, reg_y, 0))
            goto gen_err;
        save_code(m_code, virt_executer.rva_cur, true);

        if (!m_code.arifmetic_reg64_imm32(AND, reg_temp, 0xFF))
            goto gen_err;
        save_code(m_code, virt_executer.rva_cur, true);

        if (!m_code.mov_reg8_rm8(reg_temp, reg_s, 1, reg_temp, 0))
            goto gen_err;
        save_code(m_code, virt_executer.rva_cur, true);

        // xor
        if (m_code.arifmetic_dword_ptr_rm64_reg32(XOR, reg_temp, virt_executer.reg_base, 0, reg_counter, block_rva) == false)
            goto gen_err;
        save_code(m_code, virt_executer.rva_cur, true);

        // add reg_counter, 1
        if (!m_code.arifmetic_reg64_imm32(ADD, reg_counter, 1))
            goto gen_err;
        save_code(m_code, virt_executer.rva_cur, true);

        // cmp reg_counter64, block_size
        if (m_code.arifmetic_reg64_imm32(CMP, reg_counter, block_size) == false)
            goto gen_err;
        save_code(m_code, virt_executer.rva_cur, true);

        // jnz label_loop_start
        if (m_code.jCC_32(JNZ, virt_executer.rva_cur, label_rva_loop) == false)
            goto gen_err;
        save_code(m_code, virt_executer.rva_cur, true);

        // корректировка перехода в конец процедуры расшифрования
        if (m_code.jCC_32(JNZ, label_rva_check_1, virt_executer.rva_cur) == false)
            goto gen_err;
        save_code(m_code, label_rva_check_1, false);
    }

    virt_executer.include_reg_32_64(reg_x);
    virt_executer.include_reg_32_64(reg_y);
    virt_executer.include_reg_32_64(reg_s);
    virt_executer.include_reg_32_64(reg_counter);
    virt_executer.include_reg_32_64(reg_temp);

    return true;

gen_err:

    printf("[-] can't generate code. Check create_encrypted_function_rc4() !\n");
    return false;
}













	// проверка нужно ли расшифровывать? 
	// для этого проверяем случайные байты из нескольких участков
	encrypt_value32 = block_ptr[0];
	
	// cmp dword ptr [reg_base + block_rva + 0], encrypt_value32
	if (m_code.arifmetic_rm64_imm32_dword_ptr(CMP, virt_executer.reg_base, 0, REG_ERR, block_rva, encrypt_value32) == false) goto gen_err; save_code(m_code, virt_executer.rva_cur, true);
	
	label_rva_check_1 = virt_executer.rva_cur;

	// jnz 
	if (m_code.jCC_32(JNZ, label_rva_check_1, 0xffffffff) == false) goto gen_err; save_code(m_code, virt_executer.rva_cur, true);
	

	// mov reg_counter64, 0
	if (m_code.mov_reg64_imm64(reg_counter64, 0) == false) goto gen_err; save_code(m_code, virt_executer.rva_cur, true);

	// mov rnd_iv_reg64, rnd_start_iv0
	if (m_code.mov_reg64_imm64(rnd_iv_reg64, rnd_start_iv0) == false) goto gen_err; save_code(m_code, virt_executer.rva_cur, true);

	label_rva_loop = virt_executer.rva_cur;

	// xor dword ptr [reg_base + block_rva + reg_counter64*4], rnd_iv_reg64 (надо взять только 32 бита от 64 битного регистра)
	if (m_code.arifmetic_dword_ptr_rm64_reg32(XOR, rnd_iv_reg64, virt_executer.reg_base, 4, reg_counter64, block_rva) == false) goto gen_err; save_code(m_code, virt_executer.rva_cur, true);

	// add rnd_iv_reg32, rnd_add_value32
	if (m_code.arifmetic_reg64_imm32(ADD, rnd_iv_reg64, rnd_add_value32) == false) goto gen_err; save_code(m_code, virt_executer.rva_cur, true);

	// add reg_counter64, 1
	if (m_code.arifmetic_reg64_imm32(ADD, reg_counter64, 1) == false) goto gen_err; save_code(m_code, virt_executer.rva_cur, true);

	// cmp reg_counter64, block_size
	if (m_code.arifmetic_reg64_imm32(CMP, reg_counter64, block_size) == false) goto gen_err; save_code(m_code, virt_executer.rva_cur, true);

	// jnz label_loop_start
	if (m_code.jCC_32(JNZ, virt_executer.rva_cur, label_rva_loop) == false) goto gen_err; save_code(m_code, virt_executer.rva_cur, true);

	// корректировка перехода в конец процедуры расшифрования
	if (m_code.jCC_32(JNZ, label_rva_check_1, virt_executer.rva_cur) == false) goto gen_err; save_code(m_code, label_rva_check_1, false);
}
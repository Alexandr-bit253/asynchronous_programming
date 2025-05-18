#include <windows.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>
#include <random>
#include <mutex>


using namespace std;


std::mutex mx;
void setCursorPosition(short x, short y) {
	static HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
	COORD p{ x, y };
	SetConsoleCursorPosition(h, p);
}


std::string center(const string& s, int width) {
	if ((int)s.size() >= width) return s.substr(0, width);
	int pad = width - s.size();
	int left = pad / 2;
	int right = pad - left;
	return string(left, ' ') + s + string(right, ' ');
}


void worker(int id, int row, int width_bar) {
    using namespace std::chrono;
    auto start = high_resolution_clock::now();

    for (int step = 0; step <= width_bar; ++step) {
        int filled = (width_bar * step) / width_bar;
        std::string bar = std::string(filled, '=') + std::string(width_bar - filled, ' ');

        {
            std::lock_guard<std::mutex> lock(mx);
            setCursorPosition(0, static_cast<short>(row));
            std::cout
                << center(std::to_string(id), 5) << " "
                << center(bar, 30) << " "
                << center("...", 10);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    auto end = high_resolution_clock::now();
    double elapsed = duration_cast<duration<double>>(end - start).count();

    char timeStr[16];
    snprintf(timeStr, sizeof(timeStr), "%.3f", elapsed);

    {
        std::lock_guard<std::mutex> lock(mx);
        setCursorPosition(0, static_cast<short>(row));
        std::string bar(width_bar, '=');
        std::cout
            << center(std::to_string(id), 5) << " "
            << center(bar, 30) << " "
            << center(timeStr, 10);
    }
}



int main() {
	const int num_threads{ 5 };
	const int width_bar{ 40 };

	cout
		<< center("#", 5) << " "
		<< center("Progress Bar", width_bar) << " "
		<< center("Time", 10) << "\n";

	std::vector<std::thread> thr;
	for (int i = 0; i < num_threads; ++i) {
		// row = строка = 1 (заголовок) + i
		thr.emplace_back(worker, i, 1 + i, width_bar);
	}
	for (auto& t : thr) t.join();

	// 3) после завершения опустим курсор ниже таблицы
	setCursorPosition(0, static_cast<short>(num_threads + 2));

	return 0;
}


//using namespace std;
//using steady_clock = chrono::steady_clock;
//
//// Глобальный хэндл и мьютекс для потокобезопасного вывода
//static const HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
//mutex io_mutex;
//
//// Получить текущую позицию курсора
//COORD get_cursor_pos() {
//    CONSOLE_SCREEN_BUFFER_INFO csbi;
//    GetConsoleScreenBufferInfo(hConsole, &csbi);
//    return csbi.dwCursorPosition;
//}
//
//// Установить курсор в абсолютную позицию (x, y)
//void set_cursor(int x, int y) {
//    COORD pos{ SHORT(x), SHORT(y) };
//    SetConsoleCursorPosition(hConsole, pos);
//}
//
//// Рисует прогресс-бар по координатам (x, y)
//void render_progress_bar(int x, int y, int width, double progress, char fill) {
//    int filled = int(width * progress + 0.5);
//    int empty = width - filled;
//    set_cursor(x, y);
//    for (int i = 0; i < filled; ++i)  cout << fill;
//    for (int i = 0; i < empty; ++i)  cout << '.';
//}
//
//void worker(size_t index,
//    int bar_width,
//    atomic<bool>& ready,
//    int baseY,
//    int col_id_x,
//    int col_bar_x,
//    int col_time_x)
//{
//    // Ждём сигнала старта
//    while (!ready.load(memory_order_acquire))
//        this_thread::yield();
//
//    // Абсолютная строка для этого потока
//    int y = baseY + 1 + int(index);
//
//    // PRNG для случайных задержек
//    mt19937 gen(random_device{}() ^ unsigned(index));
//    uniform_int_distribution<int> dist(50, 150);
//
//    auto t0 = steady_clock::now();
//
//    // Цикл прогресса
//    for (int pos = 0; pos <= bar_width; ++pos) {
//        this_thread::sleep_for(chrono::milliseconds(dist(gen)));
//
//        // Печатаем index и id (на всякий случай можно вынести из цикла)
//        {
//            lock_guard<mutex> lk(io_mutex);
//            set_cursor(0, y);
//            cout << setw(2) << index;
//
//            set_cursor(col_id_x, y);
//            cout << setw(8) << this_thread::get_id();
//            cout.flush();
//        }
//
//        // Рендерим бар
//        double prog = double(pos) / bar_width;
//        {
//            lock_guard<mutex> lk(io_mutex);
//            render_progress_bar(col_bar_x, y, bar_width, prog, '=');
//            cout.flush();
//        }
//    }
//
//    auto t1 = steady_clock::now();
//    double secs = chrono::duration<double>(t1 - t0).count();
//
//    // Финальная перерисовка: бар заполнен + время
//    {
//        lock_guard<mutex> lk(io_mutex);
//        // индекс и id
//        set_cursor(0, y);
//        cout << setw(2) << index;
//
//        set_cursor(col_id_x, y);
//        cout << setw(8) << this_thread::get_id();
//
//        // полный бар
//        render_progress_bar(col_bar_x, y, bar_width, 1.0, '=');
//
//        // время
//        set_cursor(col_time_x, y);
//        cout << fixed << setprecision(5) << secs << "s";
//        cout.flush();
//    }
//}
//
//int main() {
//    const int num_threads = 5;
//    const int bar_width = 20;
//
//    // Вычислим X‑координаты столбцов
//    const int col_id_x = 4;                   // после "#  "
//    const int col_bar_x = col_id_x + 8 + 2;    // id + 2 пробела
//    const int col_time_x = col_bar_x + bar_width + 3;
//
//    // 1) Сохраняем базовую позицию курсора
//    COORD basePos = get_cursor_pos();
//
//    // 2) Рисуем заголовок и blank‑строки
//    set_cursor(basePos.X, basePos.Y);
//    cout << "#  " << setw(8) << "id" << "  "
//        << setw(bar_width + 2) << left << "Progress Bar"
//        << "Time\n";
//
//    for (int i = 0; i < num_threads; ++i) {
//        cout << "\n";
//    }
//    cout.flush();
//
//    // 3) Флаг старта
//    atomic<bool> ready{ false };
//
//    // 4) Запускаем worker’ы, передаём им basePos.Y
//    vector<thread> threads;
//    threads.reserve(num_threads);
//    for (size_t i = 0; i < num_threads; ++i) {
//        threads.emplace_back(
//            worker,
//            i, bar_width,
//            ref(ready),
//            basePos.Y,
//            col_id_x,
//            col_bar_x,
//            col_time_x
//        );
//    }
//
//    // 5) Дадим стартовую метку
//    ready.store(true, memory_order_release);
//
//    // 6) Ждём завершения
//    for (auto& t : threads) t.join();
//
//    int finishY = basePos.Y + 1 + num_threads;
//    set_cursor(0, finishY);
//    // Чистим эту строку и ставим пустую
//    cout << "\033[K" << '\n';
//    cout.flush();
//
//    return 0;
//}
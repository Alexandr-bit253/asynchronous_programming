#include <iostream>
#include <mutex>
#include <thread>
#include <chrono>
#include <atomic>

using namespace std;

void client_f(atomic<int>& max_count, atomic<int>& count, atomic<bool>& client_done) {
	while (count.load(memory_order_relaxed) < max_count.load(memory_order_relaxed)) {
		this_thread::sleep_for(1s);
		count.fetch_add(1, memory_order_relaxed);
		cout << "client_f   count = " << count.load(memory_order_relaxed) << endl;
	}
	client_done.store(true, memory_order_release);
}

void operator_f(atomic<int>& count, atomic<bool>& client_done) {
	while (!client_done.load(memory_order_acquire) ||
		count.load(memory_order_acquire) > 0) {
		this_thread::sleep_for(2s);
		if (count.load(memory_order_acquire) > 0) {
			count.fetch_sub(1, memory_order_acq_rel);
			cout << "operator_f count = " << count.load(memory_order_acquire) << endl;
		}
	}
}

int main() {
	atomic<int> max_count{ 10 };
	atomic<int> count{};
	atomic<bool> client_done{ false };

	thread t1(client_f, ref(max_count), ref(count), ref(client_done));
	thread t2(operator_f, ref(count), ref(client_done));

	t1.join();
	t2.join();

	return 0;
}
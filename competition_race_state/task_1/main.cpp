#include <iostream>
#include <mutex>
#include <thread>
#include <chrono>
#include <atomic>

using namespace std;

void client_f(atomic<int>& max_count, atomic<int>& count, atomic<bool>& client_done) {
	while (count < max_count) {
		this_thread::sleep_for(1s);
		++count;
		cout << "client_f   count = " << count.load() << endl;
	}
	client_done.store(true);
}

void operator_f(atomic<int>& count, atomic<bool>& client_done) {
	while (!client_done.load() || count > 0) {
		this_thread::sleep_for(2s);
		if (count > 0) {
			--count;
			cout << "operator_f count = " << count.load() << endl;
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
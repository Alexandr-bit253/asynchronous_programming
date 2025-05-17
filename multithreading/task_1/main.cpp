#include <iostream>
#include <mutex>
#include <thread>
#include <chrono>

using namespace std;

void client_f(int& max_count, int& count) {
	while (count < max_count) {
		this_thread::sleep_for(1s);
		count++;
		cout << "client_f   count = " << count << endl;
	}
}

void operator_f(int& count) {
	do {
		this_thread::sleep_for(2s);
		count--;
		cout << "operator_f count = " << count << endl;
	} while (count > 0);
}

int main() {
	int max_count{ 10 };
	int count{};

	thread t1(client_f, ref(max_count), ref(count));
	thread t2(operator_f, ref(count));

	t1.join();
	t2.join();

	return 0;
}
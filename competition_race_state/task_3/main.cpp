#include <iostream>
#include <thread>
#include <mutex>

using namespace std;


class Data {
public:
	mutex mtx;
	int value{};

	Data(int val) : value(val) {}
};


void swap_with_lock(Data& a, Data& b) {
	if (&a == &b) return;

	lock(a.mtx, b.mtx);

	swap(a.value, b.value);

	a.mtx.unlock();
	b.mtx.unlock();
}


void swap_with_scoped_lock(Data& a, Data& b) {
	if (&a == &b) return;
	scoped_lock sc_lk(a.mtx, b.mtx);
	swap(a.value, b.value);
}


void swap_with_unique_lock(Data& a, Data& b) {
	if (&a == &b) return;

	unique_lock<mutex> u_lk_a(a.mtx, defer_lock);
	unique_lock<mutex> u_lk_b(b.mtx, defer_lock);

	lock(u_lk_a, u_lk_b);

	swap(a.value, b.value);

	//u_lk_a.unlock();
	//u_lk_b.unlock();
}


int main() {
	Data d1(10), d2(20);

	cout << "data befor swap: d1 = " << d1.value << ", d2 = " << d2.value << endl;

	//swap_with_lock(d1, d2);
	//swap_with_scoped_lock(d1, d2);
	swap_with_unique_lock(d1, d2);

	cout << "data after swap: d1 = " << d1.value << ", d2 = " << d2.value << endl;

	return 0;
}
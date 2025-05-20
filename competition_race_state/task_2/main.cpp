#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <vector>
#include <random>
#include <mutex>

#include "utils.hpp"


using namespace std;


mutex mtx_console;

const int THREAD_COUNT{ 5 };
const int BAR_WIDTH{ 10 };


const int ID_X{ 2 };
const int HEADER_Y{ 0 };
const int BAR_X{ 15 };
const int TIME_X{ BAR_X + BAR_WIDTH + 3 };
const int FIRST_BAR_Y{ 2 };


void draw_header() {
	consol_parameter::SetPosition(ID_X, HEADER_Y);
	cout << "#";
	consol_parameter::SetPosition(ID_X + 4, HEADER_Y);
	cout << "id";
	consol_parameter::SetPosition(BAR_X, HEADER_Y);
	cout << "Progress Bar";
	consol_parameter::SetPosition(TIME_X + 4, HEADER_Y);
	cout << "Time";
}


void worker(int index) {
	int y{ FIRST_BAR_Y + index };

	random_device rd;
	mt19937 gen(rd());
	uniform_int_distribution<> dist(0, 1000);

	{
		lock_guard<mutex> lk(mtx_console);
		consol_parameter::SetPosition(ID_X, y);
		cout << index + 1;
		consol_parameter::SetPosition(ID_X + 4, y);
		cout << this_thread::get_id();
		consol_parameter::SetPosition(BAR_X -1, y);
		cout << "[";
		consol_parameter::SetPosition(BAR_X + BAR_WIDTH, y);
		cout << "]";
	}

	Timer timer;

	for (int i = 0; i < BAR_WIDTH; ++i) {
		int ms = dist(gen);
		this_thread::sleep_for(chrono::milliseconds(ms));
		lock_guard<mutex> lk(mtx_console);
		consol_parameter::SetPosition(BAR_X + i, y);
		cout << "=";
	}

	lock_guard<mutex> lk(mtx_console);
	consol_parameter::SetPosition(TIME_X, y);
	timer.print();
}


int main() {
	::ShowCursor(false);

	draw_header();

	vector<thread> ths;
	ths.reserve(THREAD_COUNT);

	for (int i = 0; i < THREAD_COUNT; ++i)
		ths.emplace_back(worker, i);
	

	for (auto& th : ths)
		th.join();

	consol_parameter::SetPosition(0, FIRST_BAR_Y + THREAD_COUNT + 2);

	return 0;
}
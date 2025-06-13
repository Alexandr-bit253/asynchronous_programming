#include <algorithm>
#include <iostream>
#include <vector>
#include <thread>
#include <future>


using namespace std;


void find_min_index(const vector<int>& vec, size_t start, promise<size_t> result_promise) {
	size_t min_index{ start };

	for (size_t j = start; j < vec.size(); ++j) {
		if (vec[j] < vec[min_index])
			min_index = j;
	}

	result_promise.set_value(min_index);
}


void sorting_selection(vector<int>& vec) {
	const size_t n{ vec.size() };
	for (size_t i = 0; i < n; ++i) {
		promise<size_t> min_promise;
		future<size_t> min_future = min_promise.get_future();
		auto task = async(
			find_min_index,
			cref(vec),
			i,
			move(min_promise)
		);
		
		size_t min_index{ min_future.get() };

		if (i != min_index)
			swap(vec[i], vec[min_index]);
	}
}


void print_vector(vector<int>& vec) {
	for (auto item : vec)
		cout << item << " ";
}


int main() {
	vector<int> nums{ 56, 3, 94, 8, 24, 5, 21, 34, 56, 7, 1 };

	cout << "before sort: ";
	print_vector(nums);
	cout << endl;

	sorting_selection(nums);

	cout << "after sort:  ";
	print_vector(nums);
	cout << endl;

	return 0;
}
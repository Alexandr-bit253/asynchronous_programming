#include <algorithm>
#include <iostream>
#include <iterator>
#include <vector>
#include <thread>
#include <future>

using namespace std;


constexpr const size_t MIN_BLOCK_SIZE{ 1000 };


static void cout_first_10(vector<int>& vec) {
	for (size_t i = 0; i < 10; ++i) {
		cout << vec[i] << " ";
	}
	cout << "\n";
}


template<typename Iterator, typename Func>
void parallel_for_each(Iterator first, Iterator last, Func f) {
	const size_t len = distance(first, last);
	
	if (len == 0) {
		return;
	} else if (len <= MIN_BLOCK_SIZE) {
		for_each(first, last, f);
		return;
	}

	Iterator mid{ first };
	advance(mid, len / 2);

	auto left_future = async(
		launch::async,
		[first, mid, f] {
			parallel_for_each(first, mid, f);
		}
	);

	parallel_for_each(mid, last, f);

	left_future.get();
}


int main() {
	vector<int> data(10'000, 1);

	cout << "before for_each: ";
	cout_first_10(data);

	parallel_for_each(data.begin(), data.end(), [](int& x) {
		x *= 2;
	});

	cout << "after for_each:  ";
	cout_first_10(data);

	bool all_doubled = all_of(data.begin(), data.end(), [](int x) {
		return x == 2;
	});

	cout << "all doubled: " << boolalpha << all_doubled << endl;

	return 0;
}
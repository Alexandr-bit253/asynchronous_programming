#include <iostream>
#include <iomanip>
#include <numeric>
#include <vector>
#include <thread>
#include <chrono>

using namespace std;

using Clock = chrono::high_resolution_clock;

template<typename T>
void partial_sum(
	const vector<T>& a,
	const vector<T>& b,
	vector<T>& c,
	size_t start,
	size_t end
) {
	for (size_t i = start; i < end; ++i) {
		c[i] = a[i] + b[i];
	}
}

template<typename T>
void parallel_sum(
	const vector<T>& a,
	const vector<T>& b,
	vector<T>& c,
	unsigned num_threads
) {
	size_t n = a.size();

	if (num_threads <= 1) {
		partial_sum(a, b, c, 0, n);
		return;
	}

	vector<thread> threads;
	threads.reserve(num_threads);

	size_t chunk = n / num_threads;
	for (unsigned t = 0; t < num_threads; ++t) {
		size_t start = t * chunk;
		size_t end = (t + 1 == num_threads ? n : start + chunk);
		threads.emplace_back(
			partial_sum<T>,
			cref(a),
			cref(b),
			ref(c),
			start,
			end
		);
	}
	for (auto& th : threads) {
		th.join();
	}
}


int main() {
	cout << "number of hardware cores: " << thread::hardware_concurrency() << endl;

	vector<size_t> sizes{ 1'000, 10'000, 100'000, 1'000'000 };
	vector<unsigned> thread_counts{ 1, 2, 4, 8, 16 };
	// таблица времени
	vector<vector<double>> times(thread_counts.size(), vector<double>(sizes.size()));

	// замер
	for (size_t ti = 0; ti < thread_counts.size(); ++ti) {
		unsigned n_thread = thread_counts[ti];
		for (size_t si = 0; si < sizes.size(); ++si) {
			size_t n = sizes[si];

			vector<int> A(n), B(n), C(n);
			iota(A.begin(), A.end(), 0);
			iota(B.begin(), B.end(), 10000);

			auto start = Clock::now();
			parallel_sum(A, B, C, n_thread);
			auto end = Clock::now();
			auto time = chrono::duration_cast<chrono::duration<double>>(end - start).count();

			times[ti][si] = time;
		}
	}

	// вывод таблицы
	cout << fixed << setprecision(6);
	cout << setw(8) << "Threads";
	for (auto n : sizes) {
		cout << setw(12) << n;
	}
	cout << "\n";
	for (size_t ti = 0; ti < thread_counts.size(); ++ti) {
		cout << setw(8) << thread_counts[ti];
		for (size_t si = 0; si < sizes.size(); ++si) {
			cout << setw(12) << times[ti][si];
		}
		cout << "\n";
	}

	double best_avg = numeric_limits<double>::max();
	unsigned best_threads = 0;
	for (size_t ti = 0; ti < thread_counts.size(); ++ti) {
		double sum = accumulate(times[ti].begin(), times[ti].end(), 0.0);
		double avg = sum / sizes.size();
		if (avg < best_avg) {
			best_avg = avg;
			best_threads = thread_counts[ti];
		}
	}

	cout << "best count threads: " << best_threads
		<< ", avg time: " << best_avg << " s" << endl;

	return 0;
}
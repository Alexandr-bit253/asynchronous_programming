#include <condition_variable>
#include <functional>
#include <iostream>
#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <chrono>
#include <future>


std::mutex coutMtx;


template <typename T>
class SafeQueue {
private:
	std::queue<T> myQueue;
	std::mutex mtx{};
	std::condition_variable cv{};
	bool doneQueue{ false };

public:
	SafeQueue() = default;
	~SafeQueue() = default;

	void shutdown() {
		{
			std::lock_guard<std::mutex> lock(mtx);
			doneQueue = true;

		}
		cv.notify_all();
	}

	void push(T value) {
		{
			std::lock_guard<std::mutex> lock(mtx);
			myQueue.push(std::move(value));
		}
		cv.notify_one();
	}

	bool pop(T& out) {
		std::unique_lock<std::mutex> lock(mtx);
		cv.wait(lock, [this] {return !myQueue.empty() || doneQueue; });
		if (myQueue.empty())
			return false;
		out = std::move(myQueue.front());
		myQueue.pop();
		return true;
	}
};


class ThreadPool {
private:
	std::vector<std::thread> workers;
	SafeQueue<std::function<void()>> workQueue;
	std::mutex mtxPool{};
	bool donePool{};

	void workerThread() {
		while (true) {
			std::function<void()> task;
			if (!workQueue.pop(task)) {
				return;
			}
			task();
		}
	}

public:
	ThreadPool() : donePool(false) {
		const unsigned threadCount = std::thread::hardware_concurrency();
		for (unsigned i = 0; i < threadCount; ++i) {
			workers.emplace_back(&ThreadPool::workerThread, this);
		}
	}

	~ThreadPool() {
		workQueue.shutdown();
		{
			std::lock_guard<std::mutex> lock(mtxPool);
			donePool = true;
		}
		for (auto& t : workers) {
			if (t.joinable())
				t.join();
		}
	}

	template <typename F, typename... Args>
	auto submit(F&& f, Args&&... args)
		-> std::future<typename std::result_of<F(Args...)>::type>
	{
		using resultType = typename std::result_of<F(Args...)>::type;
		auto taskPtr = std::make_shared<std::packaged_task<resultType()>>(
			std::bind(std::forward<F>(f), std::forward<Args>(args)...)
		);

		std::future<resultType> res = taskPtr->get_future();
		
		workQueue.push([taskPtr]() { (*taskPtr)(); });

		return res;
	}
};


void taskA() {
	std::lock_guard<std::mutex> lg(coutMtx);
	std::cout << "task A is executed in thread "
		<< std::this_thread::get_id() << "\n";
}


void taskB() {
	std::lock_guard<std::mutex> lg(coutMtx);
	std::cout << "task B is executed in thread "
		<< std::this_thread::get_id() << "\n";
}


void taskC() {
	std::lock_guard<std::mutex> lg(coutMtx);
	std::cout << "task C is executed in thread "
		<< std::this_thread::get_id() << "\n";
}


int main() {
	ThreadPool pool;

	for (int i = 0; i < 5; ++i) {
		std::this_thread::sleep_for(std::chrono::seconds(1));
		auto fut1 = pool.submit(taskA);
		auto fut2 = pool.submit(taskB);
		auto fut3 = pool.submit(taskC);

		fut1.get();
		fut2.get();
		fut3.get();
	}

	return 0;
}
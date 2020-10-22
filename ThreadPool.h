#pragma once
//TODO:
//	threadSafe Queue
//	function return not working
//	stop, pause
//	Force/UnForce shutdown

#include<thread>
#include<vector>
#include<queue>
#include<mutex>
#include<condition_variable>
#include<future>
#include <iostream>
#include <exception>
#include <string>

#define DEBUG_MODE
#ifdef DEBUG_MODE
#define DEBUG(msg) msg;
#else
#define DEBUG(msg)
#endif

//#define MAX_THREADS (std::thread::hardware_concurrency() - 1)
#define MAX_THREADS 50

void DTORRun()
{
	std::cout << "Poision Apple" << std::endl;
}

class ExceptionThreadPool : std::runtime_error
{
public:
	ExceptionThreadPool(const char* message)
		: std::runtime_error(message) {}

	ExceptionThreadPool(char* message)
		: std::runtime_error(message) {}

	explicit ExceptionThreadPool(const std::string& message)
	: std::runtime_error(message) {}

	const char* what() { return ::std::runtime_error::what(); }
};


/* ThreadPool class
It is not a singleton and without default nThreads for give the user option for many pools for dufferent needs. for example:
in I/O system, we need more threades than cores.
for purely computational load like scientific simulations the recommanded thread per core*/
class ThreadPool
{
public:
	ThreadPool(int _numThreads) throw()
		: m_toStop(false)
	{
		if (_numThreads >= MAX_THREADS)
			return;

		m_workersThreads.reserve(_numThreads);

		int i = 0;
		try
		{
			for (i = 0; i < _numThreads; ++i)
				m_workersThreads.emplace_back(std::thread(&ThreadPool::ThreadManager, this));
		}
		catch (...)
		{
			for (int j = 0; j < i; ++j)
				m_workersThreads[j].join();

			std::string errMsg = "thread creation error, error in thread #" + i;
			ExceptionThreadPool err(errMsg);
			throw err;
		}
	}

	//void ForceShutdown()
	//void Shutdown()
	//Isv

	ThreadPool::~ThreadPool()
	{
		m_toStop = true;//maybe slow because atomic, maybe faster to add varable bool for each thread

		//sleep?
		for (auto& thread : m_workersThreads)
		{
			Push(DTORRun);
		}
		m_condVar.notify_all();
		DEBUG(std::cout << "DTOR: notify all" << std::endl);

		//Push(DTORRun);
		//m_condVar.notify_all();

		for (auto& thread : m_workersThreads)
			thread.join();
	}

	bool ShoultBeStop() noexcept
	{
		return m_toStop;
	}

	template<class Function, class ...Args>
	std::future<typename std::result_of<Function(Args...)>::type> Push(Function &&_f, Args &&..._args) noexcept
	{
		std::packaged_task<typename std::result_of<Function(Args...)>::type()> task(std::bind(_f, _args...));
		//for verify the "poision apple" will push after the real funcs, the problem tha "poision apple" not pushed too...
		/*if (IsShoultStop())
			return task.get_future();//exception?? how user know is not succeed?*/

		auto res = task.get_future();
		{
			std::unique_lock<std::mutex> guard(m_mutex);
			m_jobQueue.push(std::packaged_task<void()>(std::move(task)));
		}
		m_condVar.notify_one();
		return res;
	}

private:
	void ThreadManager() noexcept
	{
		DEBUG(std::thread::id threadId = std::this_thread::get_id());
		DEBUG(std::cout << threadId << " was created" << std::endl);
		std::packaged_task<void() > job;

		while (!ShoultBeStop())
		{
			{
				std::unique_lock<std::mutex> guard(m_mutex);
				m_condVar.wait(guard, [this] {return !m_jobQueue.empty(); });
				DEBUG(std::cout << "KUKURIKU" << std::endl;)

				if (!m_jobQueue.size())
					continue;

				job = std::move(m_jobQueue.front());
				m_jobQueue.pop();
			}

			DEBUG(std::cout << "Job execute: " << threadId << std::endl);
			job();
		}
		DEBUG(std::cout << threadId << " was destroyed" << std::endl);
	}

private:
	std::vector<std::thread>					m_workersThreads;
	std::queue<std::packaged_task<void()>>		m_jobQueue;
	std::condition_variable						m_condVar;
	std::mutex									m_mutex;//change to jobMutex
	std::atomic<bool>							m_toPause;
	bool										m_toStop;

private:
	ThreadPool(const ThreadPool&) = delete;
	ThreadPool &operator=(const ThreadPool&) = delete;
};


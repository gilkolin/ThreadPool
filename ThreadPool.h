#pragma once

#include<thread>
#include<vector>
#include<queue>
#include<mutex>
#include<condition_variable>
#include<functional>
#include<future>
#include <iostream>
#include <windows.h>

#define MAX_THREADS 15


/* ThreadPool class
It is not a singleton and without default nThreads for give the user option for many pools for dufferent needs. for example:
in I/O system, we need more threades than cores.
for purely computational load like scientific simulations the recommanded thread per core*/
class ThreadPool
{
public:
	ThreadPool(int _numThreads)
	: m_toStop(false)
	{
		if (_numThreads >= MAX_THREADS)
			return;

		m_workersThreads.reserve(_numThreads);

		for (int i = 0; i != _numThreads; ++i)
			m_workersThreads.emplace_back(std::thread(&ThreadPool::ThreadManager, this));
	}


	ThreadPool::~ThreadPool()
	{
		m_toStop = true;//maybe slow because atomic, maybe faster to add varable bool for each thread
		m_condVar.notify_all();
	
		for (auto& thread : m_workersThreads)
			thread.join();
	}
	

	template<class Function, class ...Args>
	std::future<typename std::result_of<Function(Args...)>::type> Push(Function &&_f, Args &&..._args)
	{
		std::packaged_task<typename std::result_of<Function(Args...)>::type()> task(std::bind(_f, _args...));
		auto res = task.get_future();
		{
			std::unique_lock<std::mutex> guard(m_mutex);
			m_jobQueue.push(std::packaged_task<void()>(std::move(task)));
		}
		m_condVar.notify_one();
		return res;
	}

private:
	void ThreadManager()
	{
		std::cout << "thread was created" << std::endl;
		std::packaged_task<void() > job;

		while (!m_toStop)
		{
			{
				std::unique_lock<std::mutex> guard(m_mutex);
				m_condVar.wait(guard, [this] {return !m_jobQueue.empty(); });

				if (m_jobQueue.size() < 1)
					continue;

				job = std::move(m_jobQueue.front());
				m_jobQueue.pop();
			}

			std::cout << "Job execute" << std::endl;
			job();
		}
	}

private:
	std::vector<std::thread>					m_workersThreads;
	std::queue<std::packaged_task<void()>>		m_jobQueue;
	std::condition_variable						m_condVar;
	std::mutex									m_mutex;//change to jobMutex
	std::atomic<bool>							m_toStop;
};
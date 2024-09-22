#include "thread_pool.h"

ThreadPool::ThreadPool(size_t numThreads)
	: m_bStop(false)
{
	for (size_t i = 0; i < numThreads; ++i)
	{
		m_workers.emplace_back([this] 
		{
			while (true) 
			{
				std::function<void()> task;
				{
					std::unique_lock<std::mutex> lock(m_queueMutex);
					m_condition.wait(lock, [this] { return m_bStop ||!m_tasks.empty(); });
					if (m_bStop && m_tasks.empty())
						return;
					task = std::move(m_tasks.front());
					m_tasks.pop();
				}
				task();
			}
		});
	}
}

ThreadPool::~ThreadPool()
{
	m_bStop.store(true);
	m_condition.notify_all();
	for (std::thread& worker : m_workers)
		worker.join();
}

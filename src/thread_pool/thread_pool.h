#ifndef __THREAD_POOL_H__
#define __THREAD_POOL_H__

#include <mutex>
#include <queue>
#include <future>
#include <functional>

class ThreadPool 
{
public:
    ThreadPool(size_t threadCount);
    ~ThreadPool();
    int getThreadNum() { return m_threadCount; }

    template <class F, class... Args>
    auto enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type> 
	{
        using return_type = typename std::result_of<F(Args...)>::type;
        auto task = std::make_shared<std::packaged_task<return_type()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_tasks.emplace([task]() { (*task)(); });
        }
        m_condition.notify_one();
        return res;
    }
	
private:
    size_t m_threadCount = 0;
    std::vector<std::thread> m_workers;
    std::queue<std::function<void()>> m_tasks;
    std::mutex m_queueMutex;
    std::condition_variable m_condition;
    std::atomic<bool> m_bStop;
};
#endif

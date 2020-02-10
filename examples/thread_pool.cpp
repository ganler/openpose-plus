#include "thread_pool.hpp"

// Implementation:
simple_thread_pool::simple_thread_pool(std::size_t sz)
    : m_shared_src(std::make_shared<pool_src>()), size(sz)
{
    for (int i = 0; i < sz; ++i) {
        std::thread(
            [this](std::shared_ptr<pool_src> ptr) {
                while (true) {
                    std::function<void()> task;
                    // >>> Critical region => Begin
                    {
                        std::unique_lock<std::mutex> lock(ptr->queue_mu);
                        ptr->cv.wait(lock, [&] {
                            return ptr->shutdown or !ptr->queue.empty();
                        });
                        if (ptr->shutdown and ptr->queue.empty())
                            return;  // Conditions to let the thread go.
                        task = std::move(ptr->queue.front());
                        ptr->queue.pop();
                        ptr->n_working.fetch_add(1, std::memory_order_release);
                    }
                    // >>> Critical region => End
                    task();
                    {
                        std::unique_lock<std::mutex> lock(ptr->queue_mu);
                        ptr->n_working.fetch_add(-1, std::memory_order_acquire);
                    }
                    ptr->wait_cv.notify_one();
                }
            },
            m_shared_src)
            .detach();
    }
}

void simple_thread_pool::wait()
{
    std::unique_lock<std::mutex> lock(m_shared_src->queue_mu);
    m_shared_src->wait_cv.wait(lock, [&] {
        return m_shared_src->queue.empty() and
               m_shared_src->n_working.load(std::memory_order_relaxed) == 0;
    });
}

simple_thread_pool::~simple_thread_pool()
{
    m_shared_src->shutdown = true;
    std::atomic_signal_fence(std::memory_order_seq_cst);
    m_shared_src->cv.notify_all();
}

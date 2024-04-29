#ifndef __THREADPOOL_H_
#define __THREADPOOL_H_

#include <iostream>
#include <vector>
#include <queue>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>

class ThreadPool {
    private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;

    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;

    public:
    ThreadPool(const size_t threads) noexcept;  // Constructor
    ~ThreadPool() noexcept;                     // Destructor
    void resize(const size_t threads) noexcept; // Resizes the thread pool
    void clear(void) noexcept;                  // Clears the queued tasks that have NOT been started yet

    // Delete these functions as this class is NOT safe to be copied
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    // Task enqueueing function, returns an std::future
    template <typename F, typename... Args>
    auto enqueue(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
        if (!workers.size()) {
            throw std::runtime_error("enqueue called on empty ThreadPool");
        }

        using return_type = decltype(f(args...));

        std::shared_ptr<std::packaged_task<return_type()>> task = std::make_shared<std::packaged_task<return_type()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queue_mutex);

            if (stop)
                throw std::runtime_error("enqueue called on stopped ThreadPool");

            tasks.emplace([task]() {
                (*task)();
            });
        }

        condition.notify_one();
        return res;
    }

}; // class ThreadPool

inline void ThreadPool::clear(void) noexcept {
    std::queue<std::function<void()>> swapQ;
    tasks.swap(swapQ); // Swap with the empty queue
}

inline ThreadPool::ThreadPool(const size_t threads) noexcept : stop(false) {
    workers.resize(threads); // Reserve for below loop
    for (auto& thread : workers) {
        thread = std::thread([this] {
            while (true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(queue_mutex);

                    // Wait until there is a task in the queue or the thread pool is stopped (via destructor)
                    condition.wait(lock, [this] { return stop || !tasks.empty(); });
                    if (stop && tasks.empty()) return;

                    // Pop the next task from the queue.
                    task = std::move(tasks.front());
                    tasks.pop();
                } // Let std::unique_lock destructor be called, releasing lock
                try {
                    task();
                } catch (const std::exception& e) {
                    std::cerr << "Caught exception in thread pool task: " << e.what() << std::endl; // endl to flush output buffer
                    // TODO: Add a way to notify the main thread (or any passed callback function) when exceptions in the threads happen
                } catch (...) {
                    // In case a non-std exception is thrown
                    std::cerr << "Caught non-standard exception in thread pool task" << std::endl;
                }
            }
        });
    }
}

inline ThreadPool::~ThreadPool() noexcept {
    stop = true;

    condition.notify_all();
    for (auto& thread : workers) {
        if (thread.joinable()) { // Check shouldn't ever be false, but call just in case to guard against UB
            thread.join();
        }
    }
}

inline void ThreadPool::resize(const size_t threads) noexcept {
    // Lock the queue_mutex for the entirety of this resize function.
    std::unique_lock<std::mutex> lock(queue_mutex);
    size_t sz = workers.size();

    if (threads < sz) {
        // Need to shrink
        stop = true;
        for (size_t i = threads; i < sz; ++i) {
            condition.notify_one(); // Notify the number we need to lose with stop=true, then set stop back to false and join the to-be-removed threads
        }

        stop = false;

        for (auto it = std::next(workers.begin(), threads); it != workers.end(); ++it) {
            auto& thread = *it;
            if (thread.joinable()) { // Guard against UB
                thread.join();       // Join the threads to be removed
            }
        }

        workers.erase(workers.begin() + threads, workers.end()); // And finally erase them
        return;
    }

    if (threads > sz) {
        // Need to grow
        workers.resize(threads);
        for (auto it = std::next(workers.begin(), sz); it != workers.end(); ++it) {
            auto& thread = *it;
            thread = std::thread([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> queueLock(queue_mutex);

                        condition.wait(queueLock, [this] { return stop || !tasks.empty(); });
                        if (stop && tasks.empty()) return;

                        task = std::move(tasks.front());
                        tasks.pop();
                    }

                    try {
                        task();
                    } catch (const std::exception& e) {
                        std::cerr << "Caught exception in thread pool task: " << e.what() << std::endl;
                    } catch (...) {
                        std::cerr << "Caught non-standard exception in thread pool task" << std::endl;
                    }
                }
            });
        }
    }
}

#endif //__THREADPOOL_H_

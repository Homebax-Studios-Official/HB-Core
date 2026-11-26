// HB Core - Homebax Studios. Copyright (c) 2026 Ondrej Jurasik / Homebax Studios.
#pragma once
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace HB {

enum class TaskPriority { High = 0, Normal = 1, Low = 2 };

// Priority task scheduler on a fixed pool of worker threads.
class TaskScheduler {
public:
    explicit TaskScheduler(unsigned threads = 0);
    ~TaskScheduler();
    TaskScheduler(const TaskScheduler&) = delete;
    TaskScheduler& operator=(const TaskScheduler&) = delete;

    void post(std::function<void()> task, TaskPriority priority = TaskPriority::Normal);

    template <class F>
    auto submit(F&& f, TaskPriority priority = TaskPriority::Normal) -> std::future<decltype(f())> {
        using R = decltype(f());
        auto task = std::make_shared<std::packaged_task<R()>>(std::forward<F>(f));
        auto fut = task->get_future();
        post([task] { (*task)(); }, priority);
        return fut;
    }

    // Blocks until the queue is empty and no task is running.
    void waitIdle();
    // Stops accepting work, finishes queued tasks, joins workers. Idempotent.
    void shutdown();
    unsigned threadCount() const { return static_cast<unsigned>(workers_.size()); }

private:
    struct Item {
        int priority;
        uint64_t seq;
        std::function<void()> fn;
        bool operator<(const Item& o) const {
            if (priority != o.priority) return priority > o.priority; // lower value first
            return seq > o.seq;                                         // FIFO within priority
        }
    };
    void workerLoop();

    std::mutex mutex_;
    std::condition_variable cv_;
    std::condition_variable idleCv_;
    std::priority_queue<Item> queue_;
    std::vector<std::thread> workers_;
    uint64_t seq_ = 0;
    unsigned running_ = 0;
    bool stopping_ = false;
};

} // namespace HB

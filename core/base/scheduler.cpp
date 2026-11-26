// HB Core - Homebax Studios. Copyright (c) 2026 Ondrej Jurasik / Homebax Studios.
#include "base/scheduler.hpp"

namespace HB {

TaskScheduler::TaskScheduler(unsigned threads) {
    if (threads == 0) {
        threads = std::thread::hardware_concurrency();
        if (threads == 0) threads = 2;
        if (threads > 4) threads = 4;
    }
    for (unsigned i = 0; i < threads; ++i) workers_.emplace_back([this] { workerLoop(); });
}

TaskScheduler::~TaskScheduler() { shutdown(); }

void TaskScheduler::post(std::function<void()> task, TaskPriority priority) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stopping_) return;
        queue_.push(Item{static_cast<int>(priority), seq_++, std::move(task)});
    }
    cv_.notify_one();
}

void TaskScheduler::workerLoop() {
    for (;;) {
        Item item;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this] { return stopping_ || !queue_.empty(); });
            if (queue_.empty()) return; // stopping and drained
            item = queue_.top();
            queue_.pop();
            ++running_;
        }
        item.fn();
        {
            std::lock_guard<std::mutex> lock(mutex_);
            --running_;
            if (queue_.empty() && running_ == 0) idleCv_.notify_all();
        }
    }
}

void TaskScheduler::waitIdle() {
    std::unique_lock<std::mutex> lock(mutex_);
    idleCv_.wait(lock, [this] { return queue_.empty() && running_ == 0; });
}

void TaskScheduler::shutdown() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stopping_ && workers_.empty()) return;
        stopping_ = true;
    }
    cv_.notify_all();
    for (auto& w : workers_) if (w.joinable()) w.join();
    workers_.clear();
}

} // namespace HB

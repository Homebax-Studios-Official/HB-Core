// HB Core - Homebax Studios. Copyright (c) 2026 Ondrej Jurasik / Homebax Studios.
#pragma once
#include <functional>
#include <mutex>
#include <utility>
#include <vector>

namespace HB {

// Thread-safe observer list (event system). Handlers are invoked outside the lock, so a
// handler may connect/disconnect without deadlocking.
template <class... Args>
class Signal {
public:
    using Handler = std::function<void(Args...)>;
    using Id = unsigned long;

    Id connect(Handler handler) {
        std::lock_guard<std::mutex> lock(mutex_);
        handlers_.emplace_back(++nextId_, std::move(handler));
        return nextId_;
    }

    bool disconnect(Id id) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto it = handlers_.begin(); it != handlers_.end(); ++it) {
            if (it->first == id) { handlers_.erase(it); return true; }
        }
        return false;
    }

    void emit(Args... args) const {
        std::vector<std::pair<Id, Handler>> copy;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            copy = handlers_;
        }
        for (auto& h : copy) h.second(args...);
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return handlers_.size();
    }

private:
    mutable std::mutex mutex_;
    std::vector<std::pair<Id, Handler>> handlers_;
    Id nextId_ = 0;
};

} // namespace HB

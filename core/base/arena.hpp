// HB Core - Homebax Studios. Copyright (c) 2026 Ondrej Jurasik / Homebax Studios.
#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <new>
#include <utility>
#include <vector>

namespace HB {

// Bump allocator with destructor tracking. Objects live until the arena is destroyed or reset().
// Not thread-safe.
class Arena {
public:
    explicit Arena(size_t blockSize = 16 * 1024) : blockSize_(blockSize) {}
    ~Arena() { reset(); }
    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;

    void* allocate(size_t size, size_t align);

    template <class T, class... Args>
    T* create(Args&&... args) {
        void* mem = allocate(sizeof(T), alignof(T));
        T* obj = new (mem) T(std::forward<Args>(args)...);
        if constexpr (!std::is_trivially_destructible_v<T>) {
            dtors_.push_back({obj, [](void* p) { static_cast<T*>(p)->~T(); }});
        }
        return obj;
    }

    void reset();
    size_t bytesAllocated() const { return allocated_; }
    size_t blockCount() const { return blocks_.size(); }

private:
    struct Dtor { void* obj; void (*fn)(void*); };
    struct Block { std::unique_ptr<uint8_t[]> data; size_t size; size_t used; };
    std::vector<Block> blocks_;
    std::vector<Dtor> dtors_;
    size_t blockSize_;
    size_t allocated_ = 0;
};

} // namespace HB

// HB Core - Homebax Studios. Copyright (c) 2026 Ondrej Jurasik / Homebax Studios.
#include "base/arena.hpp"
#include <algorithm>

namespace HB {

void* Arena::allocate(size_t size, size_t align) {
    if (size == 0) size = 1;
    if (!blocks_.empty()) {
        Block& b = blocks_.back();
        uintptr_t base = reinterpret_cast<uintptr_t>(b.data.get()) + b.used;
        uintptr_t aligned = (base + (align - 1)) & ~(static_cast<uintptr_t>(align) - 1);
        size_t needed = (aligned - base) + size;
        if (b.used + needed <= b.size) {
            b.used += needed;
            allocated_ += size;
            return reinterpret_cast<void*>(aligned);
        }
    }
    size_t newSize = std::max(blockSize_, size + align);
    Block nb{std::unique_ptr<uint8_t[]>(new uint8_t[newSize]), newSize, 0};
    uintptr_t base = reinterpret_cast<uintptr_t>(nb.data.get());
    uintptr_t aligned = (base + (align - 1)) & ~(static_cast<uintptr_t>(align) - 1);
    nb.used = (aligned - base) + size;
    blocks_.push_back(std::move(nb));
    allocated_ += size;
    return reinterpret_cast<void*>(aligned);
}

void Arena::reset() {
    for (auto it = dtors_.rbegin(); it != dtors_.rend(); ++it) it->fn(it->obj);
    dtors_.clear();
    blocks_.clear();
    allocated_ = 0;
}

} // namespace HB

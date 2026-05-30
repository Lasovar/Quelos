//
// Created by luais on 28/05/2026.
//

#include "Profiling.h"
#include <cstdlib>
#include <new>

#ifdef TRACY_ENABLE

void* operator new(const std::size_t size) {
    void* p = std::malloc(size);
    TracyAlloc(p, size);
    return p;
}

void* operator new[](const std::size_t size) {
    void* p = std::malloc(size);
    TracyAlloc(p, size);
    return p;
}

void operator delete(void* p) noexcept {
    TracyFree(p);
    std::free(p);
}

void operator delete[](void* p) noexcept {
    TracyFree(p);
    std::free(p);
}

void operator delete(void* p, std::size_t) noexcept {
    TracyFree(p);
    std::free(p);
}

void operator delete[](void* p, std::size_t) noexcept {
    TracyFree(p);
    std::free(p);
}

void* operator new(std::size_t size, std::align_val_t alignment) {
    std::size_t align = static_cast<std::size_t>(alignment);

#if defined(_MSC_VER)
    void* p = _aligned_malloc(size, align);
#else
    void* p = std::aligned_alloc(align, ((size + align - 1) / align) * align);
#endif

    TracyAlloc(p, size);
    return p;
}

void* operator new[](const std::size_t size, const std::align_val_t alignment) {
    return operator new(size, alignment);
}

void operator delete(void* p, std::align_val_t) noexcept {
    TracyFree(p);

#if defined(_MSC_VER)
    _aligned_free(p);
#else
    std::free(p);
#endif
}

void operator delete[](void* p, const std::align_val_t alignment) noexcept {
    operator delete(p, alignment);
}

void operator delete(void* p, std::size_t, const std::align_val_t alignment) noexcept {
    operator delete(p, alignment);
}

void operator delete[](void* p, std::size_t, const std::align_val_t alignment) noexcept {
    operator delete(p, alignment);
}

#endif
namespace Quelos {}

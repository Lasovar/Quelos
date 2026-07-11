//
// Created by lasovar on 7/11/26.
//

#pragma once
#include <memory_resource>

#include "API.h"

namespace Quelos {
    enum class Allocator {
        None,
        Temp,
        TempJob,
        Persistent
    };

    using AllocatorType = Allocator;

    class InvalidAllocator : public std::pmr::memory_resource {
    private:
        void* do_allocate(size_t bytes, size_t align) override;
        void do_deallocate(void*, size_t, size_t) override;

        bool do_is_equal(const memory_resource& other) const noexcept override {
            return this == &other;
        }
    };

    QS_API InvalidAllocator& GetInvalidAllocator();

    class ArenaMemoryResource;
    QS_API ArenaMemoryResource& GetTempAllocator();

    QS_API std::pmr::memory_resource* GetAllocator(Allocator allocatorType);
}

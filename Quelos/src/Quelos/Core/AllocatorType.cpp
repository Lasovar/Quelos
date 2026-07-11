//
// Created by lasovar on 7/11/26.
//

#include "AllocatorType.hpp"

#include "Application.h"
#include "Base.h"
#include "Assert.h"

namespace Quelos {
    void* InvalidAllocator::do_allocate(const size_t bytes, const size_t align) {
        QS_CORE_ASSERT(false, "Uninitialized Memory Allocator!");
        return nullptr;
    }

    void InvalidAllocator::do_deallocate(void*, size_t, size_t) {
        QS_CORE_ASSERT(false, "Uninitialized Memory Allocator!");
    }

    InvalidAllocator& GetInvalidAllocator() {
        static InvalidAllocator invalidAllocator;
        return invalidAllocator;
    }

    ArenaMemoryResource& GetTempAllocator() {
        return Application::GetTempAllocator();
    }

    std::pmr::memory_resource* GetAllocator(const Allocator allocatorType) {
        switch (allocatorType) {
        case Allocator::None: return &GetInvalidAllocator();
        case Allocator::Temp:
        case Allocator::TempJob: return &GetTempAllocator();
        case Allocator::Persistent: return std::pmr::get_default_resource();
        }

        return &GetInvalidAllocator();
    }
}

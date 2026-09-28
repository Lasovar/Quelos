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

    ArenaMemoryResource& GetFrameAllocator() {
        return Application::GetTempAllocator();
    }

    std::pmr::memory_resource* GetAllocator(const Allocator allocatorType) {
        switch (allocatorType) {
        case Allocator::None: return &GetInvalidAllocator();
        case Allocator::Frame:
        case Allocator::Job: return &GetFrameAllocator();
        case Allocator::Persistent: return std::pmr::get_default_resource();
        }

        return &GetInvalidAllocator();
    }
}

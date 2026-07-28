//
// Created by lasovar on 7/11/26.
//

#include "Quelos/Platform/Allocators.hpp"

#include "windows.h"
#include "Quelos/Core/Profiling.h"

namespace Quelos::Platform {
    uint64_t GetMaxPageSize() {
        SYSTEM_INFO info;
        GetSystemInfo(&info);
        return info.dwPageSize;
    }

    PageInfo GetPageInfo() {
        SYSTEM_INFO info;
        GetSystemInfo(&info);
        return { info.dwPageSize, info.dwAllocationGranularity };
    }

    void* AllocatePages(const uint64_t size) {
        void* ptr = VirtualAlloc(
            nullptr,
            size,
            MEM_RESERVE | MEM_COMMIT,
            PAGE_READWRITE
        );

        QS_PROFILE_ALLOC_N(ptr, size, "Quelos::Platform::AllocatePages");
        return ptr;
    }

    void FreePages(void* memory, size_t) {
        VirtualFree(memory, 0, MEM_RELEASE);
        QS_PROFILE_FREE_N(memory, "Quelos::Platform::FreePages");
    }
}

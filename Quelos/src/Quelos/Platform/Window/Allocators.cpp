//
// Created by lasovar on 7/11/26.
//

#include "Quelos/Platform/Allocators.hpp"

#include "Windows.h"

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

    void* AllocatePages(uint64_t size) {
        return VirtualAlloc(nullptr,
                            size,
                            MEM_RESERVE | MEM_COMMIT,
                            PAGE_READWRITE);
    }

    void FreePages(void* memory, size_t)
    {
        VirtualFree(memory, 0, MEM_RELEASE);
    }
}

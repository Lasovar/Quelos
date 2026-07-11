//
// Created by lasovar on 7/11/26.
//

#pragma once
#include <cstdint>

namespace Quelos::Platform {
    struct PageInfo {
        uint64_t PageSize;
        uint64_t AllocationGranularity;
    };

    uint64_t GetMaxPageSize();
    PageInfo GetPageInfo();

    void* AllocatePages(uint64_t size);
    void FreePages(void* memory, uint64_t size);
}

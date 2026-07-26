//
// Created by lasovar on 7/11/26.
//

#include "Quelos/Platform/Allocators.hpp"

#include <sys/mman.h>
#include <unistd.h>

namespace Quelos::Platform {
    uint64_t GetMaxPageSize() {
        return static_cast<size_t>(sysconf(_SC_PAGESIZE));
    }

    PageInfo GetPageInfo() {
        const uint64_t pageSize = GetMaxPageSize();
        return { pageSize, pageSize };
    }

    void* AllocatePages(const uint64_t size) {
        void* ptr = mmap(
            nullptr,
            size,
            PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS,
            -1,
            0
        );

        QS_PROFILE_ALLOC_N(ptr, size, "Quelos::Platform::AllocatePages");

        return ptr;
    }

    void FreePages(void* memory, const uint64_t size) {
        munmap(memory, size);
        QS_PROFILE_FREE_N(memory, "Quelos::Platform::FreePages");
    }
}

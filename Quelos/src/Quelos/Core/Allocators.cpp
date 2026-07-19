//
// Created by lasovar on 7/11/26.
//

#include "Allocators.hpp"

#include "Quelos/Platform/Allocators.hpp"

#include "Assert.h"

namespace Quelos {
    static uint64_t g_PageSize = Platform::GetMaxPageSize();

    static Page AllocatePage(const uint64_t minSize) {
        const size_t capacity = AlignUp(std::max(minSize, g_PageSize), g_PageSize);

        return {
            .Memory = static_cast<byte*>(Platform::AllocatePages(capacity)),
            .Used = 0,
            .Capacity = capacity
        };
    }

    PageEntry* PagePool::Acquire(const uint64_t minSize) {
        PageEntry* head = m_FreeList.load(std::memory_order_acquire);

        while (head) {
            PageEntry* next = head->Next;

            if (
                m_FreeList.compare_exchange_weak(
                    head,
                    next,
                    std::memory_order_acq_rel,
                    std::memory_order_acquire
                )
            ) {
                if (head->Page.Capacity < minSize) {
                    continue;
                }

                head->Next = nullptr;
                head->Page.Used = 0;
                return head;
            }
        }

        return AllocatePageEntry(minSize);
    }

    void* Page::Allocate(const uint64_t size, const uint64_t alignment) {
        QS_CORE_ASSERT(std::has_single_bit(alignment));

        const uint64_t offset = AlignUp(Used, alignment);

        if (offset + size > Capacity) {
            return nullptr;
        }

        Used = offset + size;
        return Memory + offset;
    }

    PageEntry* PageBlock::Allocate(const uint64_t minSize) {
        PageEntry* pageEntry = m_Storage.Allocate<PageEntry>();
        if (!pageEntry) {
            return nullptr;
        }

        *pageEntry = { .Page = AllocatePage(minSize), .Next = nullptr };
        ++m_Size;

        return pageEntry;
    }

    PageBlock::~PageBlock() {
        for (uint32_t i = 0; i < m_Size; i++) {
            const PageEntry* pageEntry = GetFirstPageEntry() + i;
            Platform::FreePages(pageEntry->Page.Memory, pageEntry->Page.Capacity);
        }
    }

    PageBlock* PageBlock::Create() {
        Page page = AllocatePage(g_PageSize);
        QS_CORE_ASSERT(page.Memory);

        PageBlock* block = page.Allocate<PageBlock>();
        block->m_Storage = page;
        QS_CORE_ASSERT(block);

        return block;
    }

    void PageBlock::Destroy(PageBlock* pageBlock) {
        pageBlock->~PageBlock();
        Platform::FreePages(pageBlock->m_Storage.Memory, pageBlock->m_Storage.Capacity);
    }

    PagePool::PagePool() {
        m_PageBlocks.store(PageBlock::Create());
        m_FreeList.store(nullptr);
    }

    PagePool::~PagePool() {
        PageBlock* block = m_PageBlocks.load();

        while (block) {
            PageBlock* next = block->Next;
            PageBlock::Destroy(block);

            block = next;
        }
    }

    void PagePool::Release(PageEntry* head, PageEntry* tail) {
        PageEntry* oldHead = m_FreeList.load(std::memory_order_acquire);

        do {
            tail->Next = oldHead;
        } while (
            !m_FreeList.compare_exchange_weak(
                oldHead,
                head,
                std::memory_order_acq_rel,
                std::memory_order_acquire
            )
        );
    }

    PageEntry* PagePool::AllocatePageEntry(const uint64_t minSize) {
        PageBlock* pageBlock = m_PageBlocks.load(std::memory_order_acquire);
        PageEntry* pageEntry = pageBlock->Allocate(minSize);

        if (!pageEntry) {
            PageBlock* newHead = PageBlock::Create();

            do {
                newHead->Next = pageBlock;
            }
            while (!
                m_PageBlocks.compare_exchange_weak(
                    pageBlock,
                    newHead,
                    std::memory_order_acq_rel,
                    std::memory_order_acquire
                )
            );

            pageEntry = newHead->Allocate(minSize);
        }

        return pageEntry;
    }

    void* LinearArena::Allocate(const uint64_t size, const uint64_t alignment) {
        if (!m_Current) {
            m_Current = m_Head = m_Pool.Acquire(size);
        }

        if (void* ptr = m_Current->Page.Allocate(size, alignment)) {
            return ptr;
        }

        PageEntry* page = m_Pool.Acquire(size);

        m_Current->Next = page;
        m_Current = page;

        return page->Page.Allocate(size, alignment);
    }

    void LinearArena::Reset() {
        if (!m_Head) {
            return;
        }

        for (PageEntry* page = m_Head; page; page = page->Next) {
            page->Page.Reset();
        }

        if (PageEntry* second = m_Head->Next) {
            PageEntry* tail = m_Current;

            m_Head->Next = nullptr;
            m_Current = m_Head; // Keep at least one page

            m_Pool.Release(second, tail);
        }
    }
}

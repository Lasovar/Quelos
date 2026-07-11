//
// Created by lasovar on 7/11/26.
//

#pragma once

#include "API.h"
#include "Log.h"

#include <atomic>
#include <memory_resource>

#include "Assert.h"

namespace Quelos {

    using byte = std::byte;

    constexpr uint64_t AlignUp(const uint64_t value, const uint64_t alignment) noexcept {
        return (value + alignment - 1) & ~(alignment - 1);
    }

    constexpr uint64_t AlignDown(const uint64_t value, const uint64_t alignment) noexcept {
        return value & ~(alignment - 1);
    }

    struct QS_API Page {
        byte* Memory = nullptr;
        uint64_t Used = 0;
        uint64_t Capacity = 0;

        void* Allocate(uint64_t size, uint64_t alignment);
        void Reset() { Used = 0; }
        size_t Remaining() const { return Capacity - Used; }

        template <typename T>
        T* Allocate() { return static_cast<T*>(Allocate(sizeof(T), alignof(T))); }
    };

    struct QS_API PageEntry {
        Page Page;
        PageEntry* Next = nullptr;
    };

    struct QS_API PageBlock {
        PageBlock* Next = nullptr;

        PageBlock(const PageBlock&) = delete;
        PageBlock& operator=(const PageBlock&) = delete;

        [[nodiscard]] PageEntry* Allocate();

        [[nodiscard]] PageEntry* GetFirstPageEntry() const {
            return reinterpret_cast<PageEntry*>(
                m_Storage.Memory + AlignUp(sizeof(PageBlock), alignof(PageEntry))
            );
        }

        [[nodiscard]] uint32_t size() const { return m_Size; }

        [[nodiscard]] static PageBlock* Create();
        static void Destroy(PageBlock* pageBlock);

    private:
        PageBlock() = default;
        ~PageBlock();

    private:

        Page m_Storage;
        uint32_t m_Size = 0;
    };

    class QS_API PagePool {
    public:
        PagePool();
        ~PagePool();

        PageEntry* Acquire();
        void Release(PageEntry* head, PageEntry* tail);

    private:
        PageEntry* AllocatePageEntry();

    private:
        std::atomic<PageBlock*> m_PageBlocks;
        std::atomic<PageEntry*> m_FreeList = nullptr;
    };

    class QS_API LinearArena {
    public:
        explicit LinearArena(PagePool& pagePool)
            : m_Pool(pagePool) {}

        /// Allocates a raw memory buffer
        void* Allocate(size_t size, size_t alignment);

        /// Allocates and calls the constructor
        /// @tparam T The type to allocate
        /// @tparam Args The arguments to pass to the constructor
        /// @param args The arguments to pass to the constructor
        /// @return A pointer to the allocated object
        /// @note If the constructor acquires resources, they will be leaked since `Reset` does not call the destructor
        template <typename T, typename... Args>
        T* Create(Args&&... args) {
            void* memory = Allocate(sizeof(T), alignof(T));
            QS_CORE_ASSERT(memory);
            return new(memory) T(std::forward<Args>(args)...);
        }

        void Reset();

    private:
        PagePool& m_Pool;

        PageEntry* m_Head = nullptr;
        PageEntry* m_Current = nullptr;
    };

    class QS_API ArenaMemoryResource : public std::pmr::memory_resource {
    public:
        explicit ArenaMemoryResource(LinearArena& arena)
            : m_Arena(&arena)
        {
        }

    private:
        void* do_allocate(const size_t bytes, const size_t align) override {
            return m_Arena->Allocate(bytes, align);
        }

        void do_deallocate(void*, size_t, size_t) override {} // no-op
        bool do_is_equal(const memory_resource& other) const noexcept override {
            return this == &other;
        }

    private:
        LinearArena* m_Arena = nullptr;
    };
}

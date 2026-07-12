#pragma once

#include <cinttypes>
#include <vector>

#include "Quelos/Core/Base.h"
#include "Quelos/Utility/Generator.h"

namespace Quelos {
    template <typename Resource>
    struct Handle {
        using ResourceType = Resource;

        uint64_t Value = Invalid;

        static constexpr uint64_t Invalid = ~static_cast<uint64_t>(0);

        constexpr Handle() = default;

        [[nodiscard]] constexpr uint32_t Index() const noexcept {
            return static_cast<uint32_t>(Value);
        }

        [[nodiscard]] constexpr uint32_t Generation() const noexcept {
            return static_cast<uint32_t>(Value >> 32);
        }

        [[nodiscard]] constexpr bool IsValid() const noexcept {
            return Value != Invalid;
        }

        constexpr explicit operator bool() const noexcept {
            return IsValid();
        }

        static constexpr Handle Create(const uint32_t index, const uint32_t generation) {
            return Handle{
                static_cast<uint64_t>(generation) << 32 | index
            };
        }

        constexpr bool operator==(const Handle&) const = default;

        Handle(const uint64_t value) : Value(value) {}
        operator uint64_t() const { return Value; }
    };

    template <typename T>
    struct Slot {
        alignas(T) byte Storage[sizeof(T)] = {};

        uint32_t Generation = 0;
        uint32_t RefCount = 0;
        bool Alive = false;

        T* Get() {
            return std::launder(reinterpret_cast<T*>(Storage));
        }

        const T* Get() const {
            return std::launder(reinterpret_cast<const T*>(Storage));
        }
    };

    template <typename THandle, typename Resource>
    concept HandleFor =
        requires { typename THandle::ResourceType; }
        && std::same_as<typename THandle::ResourceType, Resource>;

    template <typename TValue, typename Resource, typename THandle>
    requires HandleFor<THandle, Resource>
    class SlotMap {
    public:
        using ValueType = TValue;
        using HandleType = THandle;

        template <typename... Args>
        THandle Emplace(Args&&... args) {
            uint32_t index;

            if (!m_FreeList.empty()) {
                index = m_FreeList.back();
                m_FreeList.pop_back();
            }
            else {
                index = static_cast<uint32_t>(m_Slots.size());
                m_Slots.emplace_back();
            }

            auto& slot = m_Slots[index];

            new (slot.Get()) TValue(std::forward<Args>(args)...);
            slot.Alive = true;
            slot.RefCount = 0;

            return THandle::Create(index, slot.Generation);
        }

        void Erase(THandle handle) {
            if (!handle.IsValid()) {
                return;
            }

            auto index = handle.Index();

            QS_CORE_ASSERT(index < m_Slots.size());

            auto& slot = m_Slots[index];

            if (slot.Generation != handle.Generation() || !slot.Alive) {
                return;
            }

            slot.Get()->~TValue();

            slot.Alive = false;
            slot.RefCount = 0;
            ++slot.Generation;

            m_FreeList.push_back(index);
        }

        TValue* At(THandle handle) {
            if (!handle.IsValid())
                return nullptr;

            auto index = handle.Index();

            if (index >= m_Slots.size()) {
                return nullptr;
            }

            auto& slot = m_Slots[index];

            if (slot.Generation != handle.Generation() || !slot.Alive) {
                return nullptr;
            }

            return slot.Get();
        }

        const TValue* At(THandle handle) const {
            if (!handle.IsValid())
                return nullptr;

            auto index = handle.Index();

            if (index >= m_Slots.size()) {
                return nullptr;
            }

            auto& slot = m_Slots[index];

            if (slot.Generation != handle.Generation() || !slot.Alive) {
                return nullptr;
            }

            return slot.Get();
        }

        void IncRef(THandle handle) {
            if (Slot<TValue>* slot = SlotAt(handle)) {
                ++slot->RefCount;
            }
        }

         bool DecRef(THandle handle) {
            if (Slot<TValue>* value = SlotAt(handle)) {
                if (--value->RefCount == 0) {
                    return true;
                }
            }

            return false;
        }

        Generator<THandle> GetAllHandles() {
            for (size_t i = 0; i < m_Slots.size(); i++) {
                if (m_Slots[i].Alive) {
                    co_yield THandle::Create(i, m_Slots[i].Generation);
                }
            }
        }

        bool IsAlive(THandle handle) const {
            return At(handle) != nullptr;
        }

        void Clear() {
            for (auto& slot : m_Slots) {
                if (slot.Alive) {
                    slot.Get()->~TValue();
                }
            }

            m_Slots.clear();
            m_FreeList.clear();
        }

        [[nodiscard]] size_t Size() const {
            return m_Slots.size() - m_FreeList.size();
        }

        [[nodiscard]] size_t Capacity() const {
            return m_Slots.size();
        }

    private:
        Slot<TValue>* SlotAt(THandle handle) {
            auto index = handle.Index();

            if (index >= m_Slots.size()) {
                return nullptr;
            }

            auto& slot = m_Slots[index];

            if (slot.Generation != handle.Generation() || !slot.Alive) {
                return nullptr;
            }

            return &m_Slots[index];
        }

    private:
        // Maybe replaces with an arena allocator/Pager?
        Deque<Slot<TValue>> m_Slots;
        Vec<uint32_t> m_FreeList{Allocator::Persistent};
    };

    template <typename Resource, typename Impl>
    using ResourceTable = SlotMap<Resource, Impl, Handle<Impl>>;
}

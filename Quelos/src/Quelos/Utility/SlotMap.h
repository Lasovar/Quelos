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

        [[nodiscard]] constexpr uint32_t GetIndex() const noexcept {
            return static_cast<uint32_t>(Value);
        }

        [[nodiscard]] constexpr uint32_t GetGeneration() const noexcept {
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
        T Value;
        uint32_t Generation = 0;
        bool Alive = false;
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

            slot.Value = TValue(std::forward<Args>(args)...);
            slot.Alive = true;

            return THandle::Create(index, slot.Generation);
        }

        void Erase(THandle handle) {
            if (!handle.IsValid()) {
                return;
            }

            auto index = handle.GetIndex();

            QS_CORE_ASSERT(index < m_Slots.size());

            auto& slot = m_Slots[index];

            if (slot.Generation != handle.GetGeneration() || !slot.Alive) {
                return;
            }

            slot.Alive = false;
            ++slot.Generation;

            m_FreeList.push_back(index);
        }

        TValue* Get(THandle handle) {
            if (!handle.IsValid())
                return nullptr;

            auto index = handle.GetIndex();

            if (index >= m_Slots.size())
                return nullptr;

            auto& slot = m_Slots[index];

            if (slot.Generation != handle.GetGeneration() || !slot.Alive) {
                return nullptr;
            }

            return &slot.Value;
        }

        const TValue* Get(THandle handle) const {
            if (!handle.IsValid())
                return nullptr;

            auto index = handle.GetIndex();

            if (index >= m_Slots.size()) {
                return nullptr;
            }

            auto& slot = m_Slots[index];

            if (slot.Generation != handle.GetGeneration() || !slot.Alive) {
                return nullptr;
            }

            return &slot.Value;
        }

        Generator<THandle> GetAllHandles() {
            for (size_t i = 0; i < m_Slots.size(); i++) {
                if (m_Slots[i].Alive) {
                    co_yield THandle::Create(i, m_Slots[i].Generation);
                }
            }
        }

        bool Alive(THandle handle) const {
            return Get(handle) != nullptr;
        }

        void Clear() {
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
        Vec<Slot<TValue>> m_Slots;
        Vec<uint32_t> m_FreeList;
    };

    template <typename Resource, typename Impl>
    using ResourceTable = SlotMap<Resource, Impl, Handle<Impl>>;
}

#pragma once

#include <cinttypes>
#include <vector>

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

        [[nodiscard]] constexpr bool Valid() const noexcept {
            return Value != Invalid;
        }

        constexpr explicit operator bool() const noexcept {
            return Valid();
        }

        static constexpr Handle Make(const uint32_t index, const uint32_t generation) {
            return Handle{
                static_cast<uint64_t>(generation) << 32 | index
            };
        }

        constexpr bool operator==(const Handle&) const = default;

        Handle(const uint64_t value) : Value(value) {}
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

            return THandle::Make(index, slot.Generation);
        }

        void Erase(THandle handle) {
            if (!handle.Valid()) {
                return;
            }

            auto index = handle.Index();

            QS_CORE_ASSERT(index < m_Slots.size());

            auto& slot = m_Slots[index];

            if (slot.Generation != handle.Generation() || !slot.Alive) {
                return;
            }

            slot.Alive = false;
            ++slot.Generation;

            m_FreeList.push_back(index);
        }

        TValue* Get(THandle handle) {
            if (!handle.Valid())
                return nullptr;

            auto index = handle.Index();

            if (index >= m_Slots.size())
                return nullptr;

            auto& slot = m_Slots[index];

            if (slot.Generation != handle.Generation() || !slot.Alive) {
                return nullptr;
            }

            return &slot.Value;
        }

        const TValue* Get(THandle handle) const {
            if (!handle.Valid())
                return nullptr;

            auto index = handle.Index();

            if (index >= m_Slots.size()) {
                return nullptr;
            }

            auto& slot = m_Slots[index];

            if (slot.Generation != handle.Generation() || !slot.Alive) {
                return nullptr;
            }

            return &slot.Value;
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

    private:
        std::vector<Slot<TValue>> m_Slots;
        std::vector<uint32_t> m_FreeList;
    };

    template <typename Resource, typename Impl>
    using ResourceTable = SlotMap<Resource, Impl, Handle<Impl>>;
}

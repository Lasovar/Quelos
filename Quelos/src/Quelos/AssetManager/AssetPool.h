#pragma once
#include "Asset.h"
#include "Quelos/Core/Assert.h"

namespace Quelos {
    struct UntypedAssetHandle {
        AssetTypeID Type = 0;
        uint32_t Index = ~0u;
        uint32_t Generation = ~0u;

        UntypedAssetHandle() = default;
        constexpr UntypedAssetHandle(const AssetTypeID type, const uint32_t index, const uint32_t generation)
            : Type(type), Index(index), Generation(generation) {}

        [[nodiscard]] bool IsValid() const {
            return Type != 0 && Index != ~0u && Generation != ~0u;
        }
    };

    template <typename T>
    struct AssetSlot {
        alignas(T) byte Storage[sizeof(T)] = {};

        uint32_t Generation = 1;
        uint32_t RefCount = 0;
        bool Constructed = false;

        T* Get() {
            return std::launder(reinterpret_cast<T*>(Storage));
        }
    };

    template <typename T>
        requires std::is_base_of_v<Asset, T>
    struct AssetHandle : UntypedAssetHandle {
        AssetHandle() = default;
        AssetHandle(const UntypedAssetHandle handle) : UntypedAssetHandle(handle) {}
    };

    template <typename T>
    requires (std::is_base_of_v<Asset, T>)
    struct AssetPool {
        Vec<AssetSlot<T>> Slots;
        Vec<uint32_t> FreeList;

        UntypedAssetHandle Allocate() {
            if (!FreeList.empty()) {
                const uint32_t index = FreeList.back();
                FreeList.pop_back();
                return { T::GetStaticType(), index, Slots[index].Generation };
            }

            const auto index = static_cast<uint32_t>(Slots.size());
            Slots.emplace_back();
            return { T::GetStaticType(), index, 1 };
        }

        void Free(UntypedAssetHandle assetHandle) {
            auto& slot = Slots[assetHandle.Index];

            if (slot.Generation != assetHandle.Generation) {
                return;
            }

            if (slot.Constructed) {
                slot.Get()->~T();
                slot.Constructed = false;
            }

            ++slot.Generation;
            slot.RefCount = 0;

            FreeList.push_back(assetHandle.Index);
        }
    };

    struct UntypedAssetPool {
        void* Data = nullptr;
        void (*DestroyPool)(void*) = nullptr;
        void* (*GetSlot)(void*, UntypedAssetHandle) = nullptr;
        void* (*GetSlotData)(void*, UntypedAssetHandle) = nullptr;
        bool (*IsValid)(void*, UntypedAssetHandle) = nullptr;
        void (*IncRef)(void*, UntypedAssetHandle) = nullptr;
        void (*DecRef)(void*, UntypedAssetHandle) = nullptr;
        void (*SetConstructed)(void*, UntypedAssetHandle, bool) = nullptr;
        void (*DestroyAt)(void*, UntypedAssetHandle) = nullptr;
        UntypedAssetHandle (*Allocate)(void*) = nullptr;

        template <typename T>
        requires (std::is_base_of_v<Asset, T>)
        static UntypedAssetPool Create() {
            auto* assetPool = new AssetPool<T>();
            UntypedAssetPool untypedPool;

            untypedPool.Data = assetPool;
            untypedPool.DestroyPool = [](void* p) {
                delete static_cast<AssetPool<T>*>(p);
            };

            untypedPool.GetSlot = [](void* pool, UntypedAssetHandle assetHandle) -> void* {
                auto* impl = static_cast<AssetPool<T>*>(pool);
                if (assetHandle.Index >= impl->Slots.size()) {
                    return nullptr;
                }

                return &impl->Slots[assetHandle.Index];
            };

            untypedPool.IsValid = [](void* pool, UntypedAssetHandle handle) -> bool {
                auto* impl = static_cast<AssetPool<T>*>(pool);
                if (handle.Index >= impl->Slots.size()) {
                    return false;
                }

                AssetSlot<T>& slot = impl->Slots[handle.Index];
                if (slot.Generation != handle.Generation) {
                    return false;
                }

                if (!slot.Constructed) {
                    return false;
                }

                return true;
            };

            untypedPool.GetSlotData = [](void* pool, UntypedAssetHandle handle) -> void* {
                auto* impl = static_cast<AssetPool<T>*>(pool);
                QS_CORE_ASSERT(handle.Index < impl->Slots.size(), "Slot index out of bounds!");
                QS_CORE_ASSERT(handle.Type == T::GetStaticType(), "Type mismatch!");

                AssetSlot<T>& slot = impl->Slots[handle.Index];

                if (slot.Generation != handle.Generation) {
                    return nullptr;
                }

                return slot.Get();
            };

            untypedPool.IncRef = [](void* pool, UntypedAssetHandle handle) {
                auto* impl = static_cast<AssetPool<T>*>(pool);
                QS_CORE_ASSERT(handle.Index < impl->Slots.size(), "Slot index out of bounds!");
                QS_CORE_ASSERT(handle.Type == T::GetStaticType(), "Type mismatch!");

                AssetSlot<T>& slot = impl->Slots[handle.Index];

                QS_CORE_ASSERT(handle.Generation == slot.Generation && slot.Constructed, "Invalid asset slot!");
                ++slot.RefCount;
            };

            untypedPool.DecRef = [](void* pool, UntypedAssetHandle handle) {
                auto* impl = static_cast<AssetPool<T>*>(pool);
                QS_CORE_ASSERT(handle.Index < impl->Slots.size(), "Slot index out of bounds!");
                QS_CORE_ASSERT(handle.Type == T::GetStaticType(), "Type mismatch!");

                AssetSlot<T>& slot = impl->Slots[handle.Index];

                QS_CORE_ASSERT(handle.Generation == slot.Generation && slot.Constructed, "Invalid asset slot!");
                QS_CORE_ASSERT(slot.RefCount > 0, "trying to free while Ref count is already 0!");

                if (--slot.RefCount == 0) {
                    impl->Free(handle);
                }
            };

            untypedPool.SetConstructed = [] (void* pool, UntypedAssetHandle handle, bool value) {
                auto* impl = static_cast<AssetPool<T>*>(pool);
                QS_CORE_ASSERT(handle.Index < impl->Slots.size(), "Slot index out of bounds!");
                QS_CORE_ASSERT(handle.Type == T::GetStaticType(), "Type mismatch!");

                AssetSlot<T>& slot = impl->Slots[handle.Index];

                QS_CORE_ASSERT(handle.Generation == slot.Generation, "Invalid asset slot!");
                slot.Constructed = value;
            };
            untypedPool.Allocate = [](void* pool) -> UntypedAssetHandle {
                return static_cast<AssetPool<T>*>(pool)->Allocate();
            };

            untypedPool.DestroyAt = [](void* pool, UntypedAssetHandle assetHandle) {
                static_cast<AssetPool<T>*>(pool)->Free(assetHandle);
            };

            return untypedPool;
        }
    };
}

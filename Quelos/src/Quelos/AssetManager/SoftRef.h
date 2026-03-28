#pragma once

#include "Quelos/AssetManager/Asset.h"
#include "Quelos/AssetManager/AssetManager.h"

namespace Quelos {
    template <typename T>
    requires (std::is_base_of_v<Asset, T>)
    class SoftRef {
    public:
        SoftRef() = default;
        SoftRef(const AssetHandle& handle) : m_AssetHandle(handle) {}

        const AssetHandle& GetAssetHandle() const { return m_AssetHandle; }
        void SetAssetHandle(const AssetHandle& handle) { m_AssetHandle = handle; }

        Ref<T>& Get() {
            if (!m_Asset) {
                if (m_AssetHandle) {
                    m_Asset = AssetManager::GetAsset<T>(m_AssetHandle);
                }
            }

            return m_Asset;
        }

        void Unload() {
            if (m_AssetHandle) {
                Project::GetAssetManager()->UnloadAsset(m_AssetHandle);
            }

            m_Asset = nullptr;
        }

        static AssetType GetAssetType() { return T::GetStaticType(); }

    private:
        Ref<T> m_Asset;
        AssetHandle m_AssetHandle;
    };
}

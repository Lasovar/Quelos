#pragma once

#include "Quelos/AssetManager/Asset.h"

namespace Quelos {
    template <typename T>
    requires (std::is_base_of_v<Asset, T>)
    class SoftRef {
    public:
        SoftRef() = default;
        explicit SoftRef(const AssetID& handle) : m_AssetId(handle) {}

        const AssetID& GetAssetID() const { return m_AssetId; }

        AssetRef<T>& Get() {
            if (!m_Asset.IsValid() && m_AssetId) {
                m_Asset = AssetRef<T>(m_AssetId);
            }

            return m_Asset;
        }

        void Unload() {
            m_Asset.Reset();
        }

        static AssetType GetAssetType() { return T::GetStaticType(); }

    private:
        AssetID m_AssetId;
        AssetRef<T> m_Asset;
    };
}

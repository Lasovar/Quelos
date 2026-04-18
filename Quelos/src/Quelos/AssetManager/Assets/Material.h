#pragma once

#include "Asset.h"
#include "Quelos/Renderer/Material.h"

namespace Quelos {
    class QS_API MaterialAsset : public Asset {
    public:
        MaterialAsset() = default;
        explicit MaterialAsset(const std::shared_ptr<::Quelos::Material>& material);

        ::Quelos::Material* GetMaterial() { return m_Material.get(); }
        const ::Quelos::Material* GetMaterial() const { return m_Material.get(); }

        static AssetType GetStaticType() { return AssetType::Material; }
        AssetType GetAssetType() const override { return GetStaticType(); }

        void SetMaterial(const std::shared_ptr<::Quelos::Material>& material) { m_Material = material; }

    private:
        std::shared_ptr<::Quelos::Material> m_Material;
    };
}

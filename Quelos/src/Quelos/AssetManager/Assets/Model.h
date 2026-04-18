#pragma once

#include "Mesh.h"

namespace Quelos {
    class QS_API Model : public Asset {
    public:
        void AddMesh(const Ref<Mesh>& mesh) { m_Meshes.push_back(mesh); }
        Vec<Ref<Mesh>>& GetMeshes() { return m_Meshes; }

        static const AssetType& GetStaticType() { return s_AssetType; }
        const AssetType& GetAssetType() const override { return s_AssetType; }
    private:
        Vec<Ref<Mesh>> m_Meshes;
    private:
        static AssetType s_AssetType;
    };
}

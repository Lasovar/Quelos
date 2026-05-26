#pragma once

#include "Quelos/AssetManager/Asset.h"
#include "Quelos/Renderer/MeshData.h"

namespace Quelos {
    class QS_API Model : public Asset {
    public:
        void AddMesh(const MeshData& mesh) { m_Meshes.push_back(mesh); }
        const Deque<MeshData>& GetMeshes() const { return m_Meshes; }
        Deque<MeshData>& GetMeshes() { return m_Meshes; }

        static const AssetType& GetStaticType() {
            static AssetType s_AssetType = Quelos::GetAssetType<Model>();
            return s_AssetType;
        }
        const AssetType& GetAssetType() const override { return GetStaticType(); }
    private:
        Deque<MeshData> m_Meshes;
    };
}

//
// Created by lasovar on 5/24/26.
//

#pragma once

#include "flecs.h"

#include "Quelos/Renderer/RenderPass.h"
#include "Quelos/Renderer/RenderResource.h"

namespace Quelos {
    struct PipelineStateComponent {
        ResourceRef<PipelineStateObject> PSO;
        ResourceRef<ShaderResourceBinding> SRB;
    };

    struct DrawCall {
        uint64_t SortKey;
        Mesh* Mesh;
        PipelineStateHandle PipelineState;
        ShaderResourceBindingHandle SRB;
        float4x4 Transform;
    };

    struct InstanceData {
        float4x4 Transform;
        uint32_t MaterialId;
        pfloat3 Padding;
    };

    class MaterialRegistry {
    public:
        // Returns the materialId to store in MeshUniforms
        uint32_t Add(const AssetRef<Material>& mat) {
            const uint32_t id = static_cast<uint32_t>(m_CPUMaterials.size());
            m_CPUMaterials.push_back(mat);
            m_Dirty = true;
            return id;
        }

        // Call once per frame (or when dirty) before drawing
        void FlushToGPU();

        void* GetSRV() { return mSRV; }
        bool WasReallocated() const { return m_Reallocated; } // signal to rebind

    private:
        Vec<AssetRef<Material>>          m_CPUMaterials;
        GPUBufferHandle         mGPUBuffer;
        void*     mSRV = nullptr;
        size_t                         mGPUCapacity = 0;
        bool                           m_Dirty       = false;
        bool                           m_Reallocated = false;
    };

    class SceneRenderer {
    public:
        SceneRenderer() = delete;
        explicit SceneRenderer(const flecs::world& world);

        void Render(const BeginRenderPassAttribs& beginRenderPassAttribs, const float4x4& viewProjection);

        ~SceneRenderer();
        RenderPassHandle GetRenderPass() const { return m_RenderPass; }

    private:
        RenderPassHandle m_RenderPass;
        flecs::world m_World;

        HashMap<AssetID, Pair<PipelineStateHandle, ShaderResourceBindingHandle>> m_PipelineStates;
        Vec<DrawCall> m_DrawCalls;

        flecs::query<const WorldTransform&, const CameraComponent&> m_CameraQuery;
        flecs::query<const WorldTransform&, const MeshRenderer&, const PipelineStateComponent&> m_RenderingQuery;
        flecs::query<const MeshRenderer&> m_PSOQuery;

        GPUBufferHandle m_GlobalBuffer;
        GPUBufferHandle m_InstanceBuffer;
    };
}

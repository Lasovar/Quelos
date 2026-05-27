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
        AssetID ShaderID;
    };

    struct DrawCommand {
        uint64_t SortKey;
        Mesh* Mesh;
        PipelineStateHandle PipelineState;
        ShaderResourceBindingHandle SRB;
        float4x4 Transform;
    };

    struct InstanceData {
        pfloat4x4 Transform;
        puint4 MaterialId;
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
        GpuBufferHandle         mGPUBuffer;
        void*     mSRV = nullptr;
        size_t                         mGPUCapacity = 0;
        bool                           m_Dirty       = false;
        bool                           m_Reallocated = false;
    };

    class SceneRenderer {
    public:
        SceneRenderer() = delete;
        explicit SceneRenderer(const flecs::world& world);

        void Begin(const BeginRenderPassAttribs& beginRenderPassAttribs, const float4x4& viewProjection);
        void Render();
        void End();

        ~SceneRenderer();
        RenderPassHandle GetRenderPass() const { return m_RenderPass; }

    private:
        RenderPassHandle m_RenderPass;
        flecs::world m_World;

        HashMap<AssetID, Pair<PipelineStateHandle, ShaderResourceBindingHandle>> m_PipelineStates;
        Vec<DrawCommand> m_DrawCalls;

        flecs::query<const WorldTransform&, const MeshRenderer&, const PipelineStateComponent&> m_RenderingQuery;
        flecs::query<const MeshRenderer&> m_PSOQuery;

        GpuBufferHandle m_GlobalBuffer;

        GpuBufferHandle m_InstancesGPUBuffer;
        Vec<InstanceData> m_InstancesCPUBuffer;
    };
}

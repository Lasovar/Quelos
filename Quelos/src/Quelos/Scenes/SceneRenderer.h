//
// Created by lasovar on 5/24/26.
//

#pragma once

#include "flecs.h"

#include "Quelos/Renderer/RenderPass.h"
#include "Quelos/Renderer/RenderResource.h"

namespace Quelos {
    class MaterialRegistry {
    public:
        MaterialRegistry(const std::string& pipelineName, const uint64_t materialSize);

        MaterialRegistry(const MaterialRegistry&) = delete;
        MaterialRegistry& operator=(const MaterialRegistry&) = delete;
        GpuBufferHandle GetGpuBufferHandle() const { return m_GPUBuffer; }

        MaterialRegistry(MaterialRegistry&& other) noexcept
            : m_GPUBuffer(other.m_GPUBuffer)
        {
            other.m_GPUBuffer = {};
        }

        MaterialRegistry& operator=(MaterialRegistry&& other) noexcept {
            if (this != &other) {
                Release();

                m_GPUBuffer = other.m_GPUBuffer;
                other.m_GPUBuffer = {};
            }

            return *this;
        }

        // Returns the materialId to store in MeshUniforms
        uint32_t Add(const AssetRef<Material>& mat) {
            if (m_MaterialSize <= 0) {
                return 0;
            }

            const auto it = std::ranges::find(m_CpuMaterials, mat);
            if (it != m_CpuMaterials.end()) {
                return static_cast<uint32_t>(it - m_CpuMaterials.begin());
            }

            const uint32_t id = static_cast<uint32_t>(m_CpuMaterials.size());
            m_CpuMaterials.push_back(mat);
            m_IsDirty = true;
            return id;
        }

        // Call once per frame (or when dirty) before drawing
        void FlushToGPU();

        bool WasReallocated() const { return m_WasReallocated; } // signal to rebind
        void Release() const;

        ~MaterialRegistry();

    private:
        std::string m_PipelineName;
        uint64_t m_MaterialSize = 0;

        Vec<AssetRef<Material>> m_CpuMaterials;
        GpuBufferHandle m_GPUBuffer;
        size_t m_GPUCapacity = 0;
        bool m_IsDirty = false;
        bool m_WasReallocated = false;
    };

    struct PipelineStateComponent {
        ResourceRef<PipelineStateObject> PSO;
        ResourceRef<ShaderResourceBinding> SRB;
        uint32_t MaterialIndex;
        AssetID ShaderID;
    };

    struct DrawCommand {
        uint64_t SortKey;
        Mesh* Mesh;
        PipelineStateHandle PipelineState;
        ShaderResourceBindingHandle SRB;
        pfloat4x4 Transform;
    };

    struct alignas(16) InstanceData {
        pfloat4x4 Transform;
        uint32_t MaterialId;
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

        struct PipelineInfo {
            PipelineStateHandle PSO;
            ShaderResourceBindingHandle SRB;
            MaterialRegistry MaterialRegistry;
        };

        HashMap<AssetID, PipelineInfo> m_PipelineStates;
        Vec<DrawCommand> m_DrawCalls;

        flecs::query<const WorldTransform&, const MeshRenderer&, const PipelineStateComponent&> m_RenderingQuery;
        flecs::query<const MeshRenderer&> m_PSOQuery;

        GpuBufferHandle m_GlobalBuffer;

        GpuBufferHandle m_InstancesGPUBuffer;
    };
}

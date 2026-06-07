//
// Created by lasovar on 5/24/26.
//

#pragma once

#include "Entity.h"
#include "flecs.h"

#include "Quelos/Renderer/RenderPass.h"
#include "Quelos/Renderer/RenderResource.h"

namespace Quelos {
    constexpr size_t k_MaxTextures = 1024;
    constexpr uint32_t k_MaxInstances = 512;

    class TextureRegistry {
    public:
        void Init(const TextureHandle whiteTexture, const TextureHandle magentaTexture) {
            m_WhiteTexture = whiteTexture;
            m_MagentaTexture = magentaTexture;
            std::ranges::fill(m_TextureArray, m_MagentaTexture.GetNativeHandle());
            m_TextureArray[k_MaxTextures - 1] = m_WhiteTexture.GetNativeHandle();
        }

        uint32_t GetID(const AssetRef<Texture2D>& textureHandle) {
            if (!textureHandle) {
                return k_MaxTextures - 1;
            }

            if (m_Textures.size() >= k_MaxTextures - 1) {
                // TODO: More data about which pipeline/material/texture lost would be nice
                QS_CORE_WARN_TAG("TextureRegistry", "TextureRegistry is full!");
                return k_MaxTextures - 1;
            }

            const uint64_t nativeHandle = textureHandle.Get().GetHandle().GetNativeHandle();
            const auto it = std::ranges::find_if(
                m_Textures,
                [&](const AssetRef<Texture2D>& texture) {
                    return texture.GetAssetID() == textureHandle.GetAssetID();
                }
            );

            if (it != m_Textures.end()) {
                const size_t id = it - m_Textures.begin();
                if (m_TextureArray[id] != it->Get().GetHandle().GetNativeHandle()) {
                    UpdateTexturesArray();
                }

                return static_cast<uint32_t>(id);
            }

            const uint32_t id = m_Textures.size();
            m_TextureArray[id] = nativeHandle;
            m_Textures.push_back(textureHandle);
            m_IsDirty = true;
            return id;
        }

        bool IsDirty() const { return m_IsDirty; }
        void SetDirty(const bool value) { m_IsDirty = value; }
        Span32<const uint64_t> GetTextureViews() { return m_TextureArray; }

        void UpdateTexturesArray() {
            for (uint32_t i = 0; i < m_Textures.size() && i < k_MaxTextures - 1; i++) {
                m_TextureArray[i] = m_Textures[i].Get().GetHandle().GetNativeHandle();
            }
        }

    private:
        Vec<AssetRef<Texture2D>> m_Textures;
        TextureHandle m_WhiteTexture;
        TextureHandle m_MagentaTexture;
        std::array<uint64_t, k_MaxTextures> m_TextureArray = {};

        bool m_IsDirty = false;
    };

    class MaterialRegistry {
    public:
        MaterialRegistry(const std::string& pipelineName, TextureRegistry& textureRegistry, uint64_t materialSize);

        [[nodiscard]] GpuBufferHandle GetGpuBufferHandle() const { return m_GPUBuffer; }

        // Returns the materialId to store in MeshUniforms
        uint32_t Add(const AssetRef<Material>& mat) {
            if (m_MaterialSize <= 0) {
                return 0;
            }

            const auto it = std::ranges::find_if(
                m_CpuMaterials,
                [&](const AssetRef<Material>& other) { return other.GetAssetID() == mat.GetAssetID(); }
            );

            if (it != m_CpuMaterials.end()) {
                return static_cast<uint32_t>(it - m_CpuMaterials.begin());
            }

            const auto id = static_cast<uint32_t>(m_CpuMaterials.size());
            m_CpuMaterials.push_back(mat);
            m_IsDirty = true;
            return id;
        }

        // Call once per frame (or when dirty) before drawing
        void FlushToGPU();

        [[nodiscard]] bool WasReallocated() const { return m_WasReallocated; } // signal to rebind
        void Release() const;

    private:
        std::string m_PipelineName;
        uint64_t m_MaterialSize = 0;

        Vec<AssetRef<Material>> m_CpuMaterials;
        GpuBufferHandle m_GPUBuffer;
        TextureRegistry* m_TextureRegistry;
        size_t m_GPUCapacity = 0;
        bool m_IsDirty = false;
        bool m_WasReallocated = false;
    };

    struct QS_API PipelineStateComponent {
        ResourceRef<PipelineStateObject> PSO;
        ResourceRef<ShaderResourceBinding> SRB;
        uint32_t MaterialIndex;
        AssetID ShaderID;
    };

    struct QS_API DrawCommand {
        uint64_t SortKey;
        Entity Entity;
        Mesh* Mesh;
        PipelineStateHandle PipelineState;
        ShaderResourceBindingHandle SRB;
        uint32_t MaterialIndex;
        pfloat4x4 Transform;
    };

    struct QS_API alignas(16) Globals {
        pfloat4x4 ViewProjection;
        pfloat4 LightDirection;
    };

    struct QS_API alignas(16) InstanceData {
        pfloat4x4 Transform;
        uint32_t MaterialId;
        uint32_t EntityIndex;
    };

    class QS_API WorldRenderer {
    public:
        WorldRenderer() = delete;
        explicit WorldRenderer(const flecs::world& world);

        void Begin(const BeginRenderPassAttribs& beginRenderPassAttribs, const float4x4& viewProjection);
        void Render();
        void End();

        [[nodiscard]] const Vec<DrawCommand>& GetDrawCalls() const { return m_DrawCalls; }
        [[nodiscard]] GpuBufferHandle GetInstancesGpuBuffer() const { return m_InstancesGPUBuffer; }
        [[nodiscard]] GpuBufferHandle GetGlobalBuffer() const { return m_GlobalBuffer; }

        ~WorldRenderer();
        [[nodiscard]] RenderPassHandle GetRenderPass() const { return m_RenderPass; }

    private:
        RenderPassHandle m_RenderPass;
        flecs::world m_World;

        TextureRegistry m_TextureRegistry;
        TextureHandle m_WhiteTexture;
        TextureHandle m_MagentaTexture;

        struct PipelineInfo {
            PipelineStateHandle PSO;
            ShaderResourceBindingHandle SRB;
            MaterialRegistry MaterialRegistry;
        };

        HashMap<AssetID, PipelineInfo> m_PipelineStates;
        Vec<DrawCommand> m_DrawCalls;

        flecs::query<const WorldTransform&, const MeshRenderer&, const PipelineStateComponent&> m_RenderingQuery;
        flecs::query<const MeshRenderer&> m_PSOQuery;
        flecs::query<const WorldTransform&, const DirectionalLight&> m_DirectionalLightQuery;

        GpuBufferHandle m_GlobalBuffer;
        GpuBufferHandle m_InstancesGPUBuffer;
    };
}

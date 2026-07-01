//
// Created by lasovar on 5/24/26.
//

#pragma once

#include "Entity.h"
#include "flecs.h"
#include "Quelos/Renderer/ComputeShader.h"

#include "Quelos/Renderer/RenderPass.h"
#include "Quelos/Renderer/RenderResource.h"

namespace Quelos {
    constexpr size_t k_MaxTextures = 128;
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

        [[nodiscard]] GpuBufferViewHandle GetBufferViewHandle() const { return m_GpuBufferView; }

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
        GpuBufferViewHandle m_GpuBufferView;
        TextureRegistry* m_TextureRegistry;
        size_t m_GPUCapacity = 0;
        bool m_IsDirty = false;
        bool m_WasReallocated = false;
    };

    struct PipelineData {
        ResourceRef<PipelineStateObject> PSO;
        ResourceRef<ShaderResourceBinding> SRB;
        int32_t Order;
    };

    struct QS_API PipelineStateComponent {
        Vec<PipelineData> Pipelines;
        uint32_t MaterialIndex;
        AssetID ShaderID;
    };

    struct QS_API DrawCommand {
        uint64_t SortKey;
        Entity Entity;
        Mesh* Mesh;
        PipelineStateHandle PipelineState;
        ShaderResourceBindingHandle SRB;
        pfloat4x4 Transform;
        uint32_t MaterialIndex;
    };

    // Designed for render passes that render the whole scene using a single PSO
    struct QS_API InstanceDrawCommand {
        pfloat4x4 Transform;
        Mesh* Mesh;
        uint32_t SortKey;
    };

    struct QS_API alignas(16) Globals {
        pfloat4x4 ViewProjection;
        pfloat4 LightDirection;
    };

    constexpr uint32_t k_NumCascades = 4;

    struct DirectionalLightShadowMap {
        ResourceRef<Texture> ShadowMaps;
        Array<ResourceRef<FrameBuffer>, k_NumCascades> ShadowFrameBuffers;
    };

    struct QS_API alignas(16) CascadeShadowData {
        pfloat4x4 LightViewProj[k_NumCascades];
        pfloat4 SplitDepths; // view-space Z end per cascade
        pfloat4x4 InvViewProjection;
        pfloat4x4 View;
    };

    struct QS_API alignas(16) InstanceData {
        pfloat4x4 Transform;
        uint32_t MaterialId;
        uint32_t EntityIndex;
    };

    struct QS_API WorldRendererView {
        ResourceRef<Texture> SceneColorMSAA;
        ResourceRef<Texture> SceneColor;
        ResourceRef<Texture> SceneDepthMSAA;

        TextureViewHandle SceneColorRTV;
        TextureViewHandle SceneColorSRV;

        TextureViewHandle SceneDepthSRV;
        TextureViewHandle SceneDepthDSV;

        ResourceRef<FrameBuffer> SceneFB;
        ResourceRef<FrameBuffer> DepthPrepassFB;

        // Shadow mask
        ResourceRef<Texture> ShadowMask;
        ResourceRef<FrameBuffer> ShadowMaskFB;

        bool ReductionReadbackReady = false;
        ResourceRef<GpuBuffer> ReductionStagingBuffer;

        float LastMinNDC = 0.0f;
        float LastMaxNDC = 1.0f;

        float4 SmoothedSplits = {5.f, 10.f, 20.f, 40.f};
    };

    struct QS_API RenderViewParams {
        float4x4 Projection;
        float4x4 View;
        float3 CameraPosition;
        Color SceneColorClear;
        float NearClip;
        float FarClip;
    };

    class QS_API WorldRenderer {
    public:
        WorldRenderer();

        void SetWorld(const flecs::world& world);

        void SetDepthReductionCompute(ComputeShader* computeShader);
        void SetShadowDepthShader(const GraphicsShader* shaderDepthShader);
        void SetShadowMaskShader(const GraphicsShader* graphicsShader);

        WorldRendererView CreateView(std::string_view name) const;
        static void ResizeView(const WorldRendererView& view, Extent2D size);

        /// View-independent, called once per frame
        void Begin();
        /// Render to a specific view (e.g GameView or SceneView)
        void Render(WorldRendererView& view, const RenderViewParams& renderViewParams) const;
        /// View-independent, called once per frame
        void End();

        [[nodiscard]] const Vec<DrawCommand>& GetDrawCalls() const { return m_DrawCalls; }
        [[nodiscard]] GpuBufferViewHandle GetInstancesBufferView() const { return m_InstancesBufferView; }
        [[nodiscard]] GpuBufferHandle GetInstancesGpuBuffer() const { return m_InstancesGpuBuffer; }
        [[nodiscard]] GpuBufferHandle GetGlobalBuffer() const { return m_GlobalBuffer; }

        ~WorldRenderer();
        [[nodiscard]] RenderPassHandle GetShadowMaskPass() const { return m_ShadowMaskRenderPass.GetHandle(); }
        [[nodiscard]] RenderPassHandle GetRenderPass() const { return m_RenderPass; }

    private:
        RenderPassHandle m_RenderPass;

        TextureRegistry m_TextureRegistry;
        TextureHandle m_WhiteTexture;
        TextureHandle m_MagentaTexture;

        RenderPassHandle m_ShadowRenderPass;
        ComputeShader* m_DepthReductionCompute;

        struct WeakPipelineData {
            PipelineStateHandle PSO;
            ShaderResourceBindingHandle SRB;
            int32_t Order = 0;
            bool HasTextures = false;

            WeakPipelineData() = default;

            WeakPipelineData(
                const PipelineStateHandle pso,
                const ShaderResourceBindingHandle srb,
                const int32_t order,
                const bool hasTextures
            )
                : PSO(pso), SRB(srb), Order(order), HasTextures(hasTextures) {}
        };

        struct PipelineInfo {
            Vec<WeakPipelineData> Pipelines;
            MaterialRegistry MaterialRegistry;
        };

        HashMap<AssetID, PipelineInfo> m_PipelineStates;
        Vec<DrawCommand> m_DrawCalls;
        Vec<InstanceDrawCommand> m_InstancingDrawCalls;

        const flecs::world* m_World = nullptr;
        flecs::query<const DirectionalLight&> m_DirectionalLightCreateSMQuery;
        flecs::query<const WorldTransform&, const DirectionalLightShadowMap&> m_DirectionalLightSMQuery;
        flecs::query<const WorldTransform&, const MeshRenderer&, const PipelineStateComponent&> m_RenderingQuery;
        flecs::query<const MeshRenderer&> m_PSOQuery;
        flecs::query<const WorldTransform&, const DirectionalLight&> m_DirectionalLightQuery;

        GpuBufferHandle m_GlobalBuffer;
        GpuBufferHandle m_InstancesGpuBuffer;
        GpuBufferViewHandle m_InstancesBufferView;

        ResourceRef<PipelineStateObject> m_ShadowComputePSO;
        ResourceRef<ShaderResourceBinding> m_ShadowComputeSRB;

        ResourceRef<GpuBuffer> m_ReductionOutBuffer;

        ResourceRef<GpuBuffer> m_LightViewProjectionBuffer;
        ResourceRef<PipelineStateObject> m_ShadowDepthPSO;
        ResourceRef<ShaderResourceBinding> m_ShadowDepthSRB;

        ResourceRef<GpuBuffer> m_CascadeShadowDataBuffer;
        ResourceRef<RenderPass> m_ShadowMaskRenderPass;
        ResourceRef<PipelineStateObject> m_ShadowMaskPSO;
        ResourceRef<ShaderResourceBinding> m_ShadowMaskSRB;
    };
}

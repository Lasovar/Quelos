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
    class WorldRenderer;
    constexpr size_t k_MaxTextures = 16;
    constexpr uint32_t k_MaxInstances = 512;

    class TextureRegistry {
    public:
        void Init(const TextureHandle whiteTexture, const TextureHandle magentaTexture) {
            m_WhiteTexture = whiteTexture;
            m_MagentaTexture = magentaTexture;
            std::ranges::fill(m_TextureArray, m_MagentaTexture.GetNativeHandle());
            m_TextureArray[k_MaxTextures - 1] = m_WhiteTexture.GetNativeHandle();
            m_IsDirty = true;
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
        Span32<const uint64_t> GetTextureViews() const { return m_TextureArray; }

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
        MaterialRegistry(const std::string& pipelineName, uint64_t materialSize);

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

        TextureRegistry& GetTextureRegistry() { return m_TextureRegistry; }
        const TextureRegistry& GetTextureRegistry() const { return m_TextureRegistry; }

    private:
        std::string m_PipelineName;
        uint64_t m_MaterialSize = 0;

        Vec<AssetRef<Material>> m_CpuMaterials;
        GpuBufferHandle m_GPUBuffer;
        GpuBufferViewHandle m_GpuBufferView;
        TextureRegistry m_TextureRegistry;
        size_t m_GPUCapacity = 0;
        bool m_IsDirty = false;
        bool m_WasReallocated = false;
    };

    struct QS_API CheckedMeshRenderer { };
    struct QS_API PipelineOf { };
    struct QS_API PipelineHandleComponent {
        ResourceRef<PipelineStateObject> PSO;
        int32_t Order;
        uint32_t MaterialIndex;
        AssetID ShaderID;
    };

    struct QS_API DepthWriteTag { };

    struct QS_API DrawCommand {
        uint64_t SortKey;
        Entity Entity;
        Mesh* Mesh;
        PipelineStateHandle PipelineState;
        pfloat4x4 Transform;
        uint32_t MaterialIndex;
    };

    // Designed for render passes that render the whole scene using a single PSO
    struct QS_API InstanceDrawCommand {
        pfloat4x4 Transform;
        Mesh* Mesh;
        uint32_t SortKey;
        // temp
        bool DepthWrite;
    };

    struct QS_API alignas(16) Globals {
        pfloat4x4 ViewProjection;
        pfloat4 LightDirection;
    };

    constexpr uint32_t k_NumCascades = 4;

    struct DirectionalLightShadowMapTag { };

    struct DirectionalLightShadowMap {
        ResourceRef<Texture> ShadowMaps;
        Array<ResourceRef<FrameBuffer>, k_NumCascades> ShadowFrameBuffers;
    };

    struct QS_API alignas(16) CascadeShadowData {
        pfloat4x4 LightViewProj[k_NumCascades];
        pfloat4 SplitDepths; // view-space Z end per cascade
        pfloat4x4 InvViewProjection;
        pfloat4x4 View;
        pfloat4 LightDirection;
    };

    struct QS_API alignas(16) InstanceData {
        pfloat4x4 Transform;
        uint32_t MaterialId;
        uint32_t EntityIndex;
    };

    /// a WorldRenderer's per-view resources
    struct QS_API WorldRendererView {
        WorldRendererView() = default;

        WorldRendererView(const WorldRendererView& other) = delete;
        WorldRendererView& operator=(const WorldRendererView& other) = delete;
        WorldRendererView(WorldRendererView&& other) = default;
        WorldRendererView& operator=(WorldRendererView&& other) = default;

        explicit WorldRendererView(const WorldRenderer* worldRenderer, const uint32_t id)
            : m_WorldRenderer(worldRenderer), m_ViewID(id) {}

        Extent2D Size;

        ResourceRef<Texture> SceneColorMSAA;
        ResourceRef<Texture> SceneColor;
        ResourceRef<Texture> SceneDepthMSAA;
        ResourceRef<Texture> SceneNormalMSAA;

        TextureViewHandle SceneColorRTV;
        TextureViewHandle SceneColorSRV;

        TextureViewHandle SceneDepthSRV;
        TextureViewHandle SceneDepthDSV;

        TextureViewHandle SceneNormalRTV;
        TextureViewHandle SceneNormalSRV;

        ResourceRef<FrameBuffer> SceneFB;
        ResourceRef<FrameBuffer> DepthPrepassFB;

        /// Data bound to each pipeline state that needs to be unique per view (e.g ShadowMaps)
        HashMap<PipelineStateHandle, ResourceRef<ShaderResourceBinding>> ViewSRBs;

        // Shadow mask
        ResourceRef<Texture> ShadowMask;
        ResourceRef<FrameBuffer> ShadowMaskFB;
        ResourceRef<ShaderResourceBinding> ShadowMaskSRB;
        bool ShadowMapsBound = false;

        bool ReductionReadbackReady = false;
        ResourceRef<GpuBuffer> ReductionStagingBuffer;
        ResourceRef<ShaderResourceBinding> ShadowComputeSRB;

        float LastMinNDC = 0.0f;
        float LastMaxNDC = 1.0f;

        float4 SmoothedSplits = {5.f, 10.f, 20.f, 40.f};

        void Resize(Extent2D size) const;

        [[nodiscard]] uint32_t GetViewID() const { return m_ViewID; }

    private:
        const WorldRenderer* m_WorldRenderer = nullptr;
        uint32_t m_ViewID = ~0u;
    };

    struct QS_API RenderViewParams {
        float4x4 Projection;
        float4x4 View;
        float3 CameraPosition;
        Color SceneColorClear;
        float NearClip = 0.0f;
        float FarClip = 0.0f;
    };

    class QS_API WorldRenderer {
    public:
        WorldRenderer();

        WorldRenderer(const WorldRenderer&) = delete;
        WorldRenderer& operator=(const WorldRenderer&) = delete;
        WorldRenderer(WorldRenderer&&) = default;
        WorldRenderer& operator=(WorldRenderer&&) = default;

        void SetWorld(const flecs::world& world);

        void SetDepthPrepassShader(const GraphicsShader* shader);
        void SetDepthReductionCompute(ComputeShader* computeShader);
        void SetShadowDepthShader(const GraphicsShader* shaderDepthShader);
        void SetShadowMaskShader(const GraphicsShader* graphicsShader);

        [[nodiscard]] const WorldRendererView* CreateView(std::string_view name, Extent2D size);
        void ResizeView(const WorldRendererView* worldRendererView, Extent2D size) const;

        /// View-independent, called once per frame
        void Begin();
        /// Render to a specific view (e.g GameView or SceneView)
        void Render(const WorldRendererView* worldRendererView, const RenderViewParams& renderViewParams) const;
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
        struct WorldRendererPipeline { };
        struct WorldRendererSystem { };

        struct WeakPipelineData {
            PipelineStateHandle PSO;
            int32_t Order = 0;
            bool HasTextures:1 = false;
            bool DepthWrite:1 = false;
            bool HasShadowMask:1 = false;
            bool IsShadowMaskBound:1 = false;
        };

        struct PipelineInfo {
            Vec<WeakPipelineData> Pipelines;
            MaterialRegistry MaterialRegistry;
        };

        void CreatePerViewResources(const Scope<WorldRendererView>& view, const MaterialRegistry& materialRegistry, const WeakPipelineData& pipeline) const;

    private:
        Vec<Scope<WorldRendererView>> m_ActiveViews;

        RenderPassHandle m_RenderPass;

        TextureHandle m_WhiteTexture;
        TextureHandle m_MagentaTexture;

        ResourceRef<RenderPass> m_DepthPrepass;

        RenderPassHandle m_ShadowRenderPass;
        ComputeShader* m_DepthReductionCompute;

        HashMap<AssetID, PipelineInfo> m_PipelineStates;
        HashMap<EntityID, DirectionalLightShadowMap> m_ShadowMaps;

        Vec<DrawCommand> m_DrawCalls;
        Vec<InstanceDrawCommand> m_InstancingDrawCalls;

        const flecs::world* m_World = nullptr;
        flecs::query<const DirectionalLight&, const EntityID&> m_DirectionalLightCreateSMQuery;
        flecs::query<const WorldTransform&, const EntityID&> m_DirectionalLightSMQuery;
        flecs::query<const WorldTransform&, const MeshRenderer&, const PipelineHandleComponent&> m_RenderingQuery;
        flecs::query<const MeshRenderer&> m_PSOQuery;
        flecs::query<const WorldTransform&, const DirectionalLight&> m_DirectionalLightQuery;

        GpuBufferHandle m_GlobalBuffer;
        GpuBufferHandle m_InstancesGpuBuffer;
        GpuBufferViewHandle m_InstancesBufferView;

        ResourceRef<PipelineStateObject> m_DepthPrepassPSO;
        ResourceRef<ShaderResourceBinding> m_DepthPrepassSRB;

        ResourceRef<PipelineStateObject> m_ShadowComputePSO;

        ResourceRef<GpuBuffer> m_ReductionOutBuffer;

        ResourceRef<GpuBuffer> m_ViewProjectionBuffer;
        ResourceRef<PipelineStateObject> m_ShadowDepthPSO;
        ResourceRef<ShaderResourceBinding> m_ShadowDepthSRB;

        ResourceRef<GpuBuffer> m_CascadeShadowDataBuffer;
        ResourceRef<RenderPass> m_ShadowMaskRenderPass;
        ResourceRef<PipelineStateObject> m_ShadowMaskPSO;
        flecs::entity m_WorldRendererPipeline;
    };
}

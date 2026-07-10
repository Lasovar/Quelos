//
// Created by lasovar on 5/24/26.
//

#include "WorldRenderer.h"

#include "magic_enum/magic_enum.hpp"
using namespace magic_enum::bitwise_operators;

#include "Quelos/Renderer/Renderer.h"

namespace Quelos {
    MaterialRegistry::MaterialRegistry(
        const std::string& pipelineName,
        const uint64_t materialSize
    )
        : m_PipelineName(pipelineName), m_MaterialSize(materialSize)
    {
    }

    void MaterialRegistry::FlushToGPU() {
        if (m_MaterialSize <= 0) {
            return;
        }

        m_IsDirty = false;
        m_WasReallocated = false;

        if (const uint64_t needed = m_CpuMaterials.size(); needed > m_GPUCapacity) {
            // Double capacity to avoid frequent reallocations
            m_GPUCapacity = math::max(needed, m_GPUCapacity * 2);
            if (m_GPUCapacity == 0) {
                m_GPUCapacity = 16; // minimum
            }

            GpuBufferSpec desc{};
            const std::string name = m_PipelineName + " Material buffer";
            desc.Name = name;
            desc.Size = m_MaterialSize * m_GPUCapacity;
            desc.Usage = Usage::Dynamic;
            desc.BindFlags = Bind::ShaderResource;
            desc.Mode = GpuBufferMode::Structured;
            desc.CpuAccessFlags = CpuAccess::Write;
            desc.ElementByteStride = m_MaterialSize;

            if (m_GPUBuffer.IsValid()) {
                Renderer::Destroy(m_GPUBuffer);
                m_GPUBuffer = {};
            }

            m_GPUBuffer = Renderer::CreateBuffer(desc, {});
            m_GpuBufferView = Renderer::GetDefaultBufferView(m_GPUBuffer, BufferViewType::ShaderResource);

            void* mapped;
            Renderer::Map(m_GPUBuffer, Map::Write, MapFlags::Discard, mapped);

            byte* materials = static_cast<byte*>(mapped);
            for (uint32_t i = 0; i < m_CpuMaterials.size(); i++) {
                const AssetRef<Material>& cpuMaterial = m_CpuMaterials[i];

                if (!cpuMaterial) {
                    continue;
                }

                const Vec<MaterialPropertySpec>& specs = cpuMaterial->GetMaterialProperties();
                const Vec<MaterialPropertyValue>& values = cpuMaterial->GetMaterialPropertyValues();

                if (specs.size() != values.size()) {
                    continue;
                }

                for (uint32_t propertyIndex = 0; propertyIndex < specs.size(); propertyIndex++) {
                    // Specialization for textures
                    if (const auto* texture = std::get_if<AssetRef<Texture2D>>(&values[propertyIndex])) {
                        uint32_t textureId = m_TextureRegistry.GetID(*texture);

                        std::memcpy(
                            materials + i * m_MaterialSize + specs[propertyIndex].Offset,
                            &textureId,
                            specs[propertyIndex].Size
                        );

                        continue;
                    }

                    const void* ptr = std::visit(
                        [](const auto& value) -> const void* {
                            return static_cast<const void*>(&value);
                        },
                        values[propertyIndex]
                    );

                    std::memcpy(
                        materials + i * m_MaterialSize + specs[propertyIndex].Offset,
                        ptr,
                        specs[propertyIndex].Size
                    );
                }
            }

            Renderer::Unmap(m_GPUBuffer, Map::Write);

            m_WasReallocated = true;
        }
        else {
            void* mapped;
            Renderer::Map(m_GPUBuffer, Map::Write, MapFlags::Discard, mapped);

            byte* materials = static_cast<byte*>(mapped);
            for (uint32_t i = 0; i < m_CpuMaterials.size(); i++) {
                const AssetRef<Material>& cpuMaterial = m_CpuMaterials[i];

                if (!cpuMaterial) {
                    continue;
                }

                const Vec<MaterialPropertySpec>& specs = cpuMaterial->GetMaterialProperties();
                const Vec<MaterialPropertyValue>& values = cpuMaterial->GetMaterialPropertyValues();

                if (specs.size() != values.size()) {
                    continue;
                }

                for (uint32_t propertyIndex = 0; propertyIndex < specs.size(); propertyIndex++) {
                    // Specialization for textures
                    if (const auto* texture = std::get_if<AssetRef<Texture2D>>(&values[propertyIndex])) {
                        uint32_t textureId = m_TextureRegistry.GetID(*texture);

                        std::memcpy(
                            materials + i * m_MaterialSize + specs[propertyIndex].Offset,
                            &textureId,
                            specs[propertyIndex].Size
                        );

                        continue;
                    }

                    const void* ptr = std::visit(
                        [](const auto& value) -> const void* {
                            return static_cast<const void*>(&value);
                        },
                        values[propertyIndex]
                    );

                    std::memcpy(
                        materials + i * m_MaterialSize + specs[propertyIndex].Offset,
                        ptr,
                        specs[propertyIndex].Size
                    );
                }
            }

            Renderer::Unmap(m_GPUBuffer, Map::Write);
        }
    }

    void MaterialRegistry::Release() const {
        if (m_GPUBuffer.IsValid()) {
            Renderer::Destroy(m_GPUBuffer);
        }
    }

    void WorldRendererView::Resize(const Extent2D size) const {
        m_WorldRenderer->ResizeView(this, size);
    }

    WorldRenderer::WorldRenderer() {
        RenderPassAttachmentSpec attachments[3];
        attachments[0].Format = ImageFormat::RGBA8UNorm;
        attachments[0].SampleCount = 4;
        attachments[0].LoadOp = AttachmentLoadOp::Clear;
        attachments[0].StoreOp = AttachmentStoreOp::Discard;
        attachments[0].InitialState = ResourceState::RenderTarget;
        attachments[0].FinalState = ResourceState::RenderTarget;

        attachments[1].Format = ImageFormat::RGBA8UNorm;
        attachments[1].SampleCount = 1;
        attachments[1].LoadOp = AttachmentLoadOp::Clear;
        attachments[1].StoreOp = AttachmentStoreOp::Store;
        attachments[1].InitialState = ResourceState::ResolveDest;
        attachments[1].FinalState = ResourceState::ShaderResource;

        attachments[2].Format = ImageFormat::Depth32Float;
        attachments[2].SampleCount = 4;
        attachments[2].LoadOp = AttachmentLoadOp::Load;
        attachments[2].StoreOp = AttachmentStoreOp::Store;
        attachments[2].InitialState = ResourceState::DepthRead;
        attachments[2].FinalState = ResourceState::DepthRead;

        AttachmentReference colorRef = {0, ResourceState::RenderTarget};
        AttachmentReference resolveRef = {1, ResourceState::ResolveDest};
        AttachmentReference depthAttachment = {2, ResourceState::DepthRead};

        SubPassSpec subPassSpec;
        subPassSpec.RenderTargetAttachments = Span32(&colorRef, 1);
        subPassSpec.pDepthAttachment = &depthAttachment;
        subPassSpec.pResolveAttachments = &resolveRef;

        RenderPassSpec renderPassSpec;
        renderPassSpec.Name = "Scene render pass";
        renderPassSpec.SubPasses = Span32(&subPassSpec, 1);
        renderPassSpec.Attachments = attachments;

        m_RenderPass = Renderer::CreateRenderPass(renderPassSpec);

        GpuBufferSpec spec;
        spec.Name = "global";
        spec.Size = sizeof(Globals);
        spec.Usage = Usage::Default;
        spec.BindFlags = Bind::UniformBuffer;

        m_GlobalBuffer = Renderer::CreateBuffer(spec, {});

        GpuBufferSpec instancesBufferSpec{};
        instancesBufferSpec.Name = "Instances";
        instancesBufferSpec.Size = sizeof(InstanceData) * k_MaxInstances;
        instancesBufferSpec.Usage = Usage::Dynamic;
        instancesBufferSpec.BindFlags = Bind::ShaderResource;
        instancesBufferSpec.Mode = GpuBufferMode::Structured;
        instancesBufferSpec.CpuAccessFlags = CpuAccess::Write;
        instancesBufferSpec.ElementByteStride = sizeof(InstanceData);

        m_InstancesGpuBuffer = Renderer::CreateBuffer(instancesBufferSpec, {});
        m_InstancesBufferView = Renderer::GetDefaultBufferView(m_InstancesGpuBuffer, BufferViewType::ShaderResource);

        TextureSpecification whiteSpec;
        whiteSpec.Name = "DefaultWhiteTexture";
        whiteSpec.BindFlags = Bind::ShaderResource;
        whiteSpec.Width = 1;
        whiteSpec.Height = 1;
        whiteSpec.Format = ImageFormat::RGBA8UNorm;
        whiteSpec.SampleCount = SampleCount::x1;
        whiteSpec.Type = TextureType::Texture2D;

        uint32_t whiteColor = 0xFFFFFFFF;
        m_WhiteTexture = Renderer::CreateTexture(whiteSpec, std::as_bytes(Span(&whiteColor, 1)));

        Renderer::TransitionResource(m_WhiteTexture, ResourceState::ShaderResource);

        TextureSpecification magentaSpec;
        magentaSpec.Name = "UndefinedTexture";
        magentaSpec.BindFlags = Bind::ShaderResource;
        magentaSpec.Width = 1;
        magentaSpec.Height = 1;
        magentaSpec.Format = ImageFormat::RGBA8UNorm;
        magentaSpec.SampleCount = SampleCount::x1;
        magentaSpec.Type = TextureType::Texture2D;

        uint32_t magentaColor = 0xFF00FFFF;
        m_MagentaTexture = Renderer::CreateTexture(magentaSpec, std::as_bytes(Span(&magentaColor, 1)));

        Renderer::TransitionResource(m_MagentaTexture, ResourceState::ShaderResource);

        // Depth Render Pass
        RenderPassAttachmentSpec depthPassAttachment[2];
        depthPassAttachment[0].Format = ImageFormat::Depth32Float;
        depthPassAttachment[0].SampleCount = 4;
        depthPassAttachment[0].LoadOp = AttachmentLoadOp::Clear;
        depthPassAttachment[0].StoreOp = AttachmentStoreOp::Store;
        depthPassAttachment[0].InitialState = ResourceState::DepthWrite;
        depthPassAttachment[0].FinalState = ResourceState::DepthRead;

        depthPassAttachment[1].Format = ImageFormat::RGBA16Float;
        depthPassAttachment[1].SampleCount = 4;
        depthPassAttachment[1].LoadOp = AttachmentLoadOp::Clear;
        depthPassAttachment[1].StoreOp = AttachmentStoreOp::Store;
        depthPassAttachment[1].InitialState = ResourceState::RenderTarget;
        depthPassAttachment[1].FinalState = ResourceState::ShaderResource;

        AttachmentReference depthRef = {0, ResourceState::DepthWrite};
        AttachmentReference normalRef = {1, ResourceState::RenderTarget};

        SubPassSpec depthSubPass[1];
        depthSubPass[0].RenderTargetAttachments = Span32(&normalRef, 1);
        depthSubPass[0].pDepthAttachment = &depthRef;

        RenderPassSpec depthPrepassSpec;
        depthPrepassSpec.Name = "DepthPrepass";
        depthPrepassSpec.SubPasses = depthSubPass;
        depthPrepassSpec.Attachments = depthPassAttachment;

        m_DepthPrepass = Renderer::CreateRenderPass(depthPrepassSpec);

        // Shadow Render Pass
        RenderPassAttachmentSpec shadowPassAttachment[1];
        shadowPassAttachment[0].Format = ImageFormat::Depth32Float;
        shadowPassAttachment[0].SampleCount = 1;
        shadowPassAttachment[0].LoadOp = AttachmentLoadOp::Clear;
        shadowPassAttachment[0].StoreOp = AttachmentStoreOp::Store;
        shadowPassAttachment[0].InitialState = ResourceState::DepthWrite;
        shadowPassAttachment[0].FinalState = ResourceState::DepthWrite;

        AttachmentReference shadowDepthRef = {0, ResourceState::DepthWrite};

        SubPassSpec shadowSubPass[1];
        shadowSubPass[0].pDepthAttachment = &shadowDepthRef;

        RenderPassSpec shadowPass;
        shadowPass.Name = "Shadow Render Pass";
        shadowPass.SubPasses = shadowSubPass;
        shadowPass.Attachments = shadowPassAttachment;

        m_ShadowRenderPass = Renderer::CreateRenderPass(shadowPass);

        // Shadow Mask

        GpuBufferSpec cascadeShadowUBSpec;
        cascadeShadowUBSpec.Name = "CascadeShadowData";
        cascadeShadowUBSpec.Size = sizeof(CascadeShadowData);
        cascadeShadowUBSpec.Usage = Usage::Default;
        cascadeShadowUBSpec.BindFlags = Bind::UniformBuffer;

        m_CascadeShadowDataBuffer = Renderer::CreateBuffer(cascadeShadowUBSpec, {});

        // Shadow Mask Pass
        RenderPassAttachmentSpec shadowMaskAttachment;
        shadowMaskAttachment.Format = ImageFormat::R8UNorm;
        shadowMaskAttachment.SampleCount = 1;
        shadowMaskAttachment.LoadOp = AttachmentLoadOp::Clear;
        shadowMaskAttachment.StoreOp = AttachmentStoreOp::Store;
        shadowMaskAttachment.InitialState = ResourceState::RenderTarget;
        shadowMaskAttachment.FinalState = ResourceState::ShaderResource;

        AttachmentReference shadowMask{ 0, ResourceState::RenderTarget };

        SubPassSpec shadowMaskSubpass{};
        shadowMaskSubpass.RenderTargetAttachments = Span32(&shadowMask, 1);

        RenderPassSpec shadowMaskPassSpec{};
        shadowMaskPassSpec.Name = "ShadowMaskPass";
        shadowMaskPassSpec.Attachments = Span32(&shadowMaskAttachment, 1);
        shadowMaskPassSpec.SubPasses = Span32(&shadowMaskSubpass, 1);

        m_ShadowMaskRenderPass = Renderer::CreateRenderPass(shadowMaskPassSpec);
    }

    void WorldRenderer::SetWorld(const flecs::world& world) {
        /* TODO: Not sure if this is needed
        if (m_RenderingQuery) {
            m_RenderingQuery.destruct();
            m_PSOQuery.destruct();
            m_DirectionalLightQuery.destruct();
        }
        */

        // ReSharper disable once CppExpressionWithoutSideEffects
        world.component<PipelineOf>()
             .add(flecs::OnDeleteTarget, flecs::Delete)
             .add(flecs::Traversable);

        m_RenderingQuery = world.query_builder<const WorldTransform&, const MeshRenderer&, const PipelineHandleComponent
                                               &>()
                                .term_at(0).up<PipelineOf>()
                                .term_at(1).up<PipelineOf>()
                                .build();

        m_WorldRendererPipeline = world.pipeline()
                                       .with(flecs::System)
                                       .with<WorldRendererSystem>()
                                       .build();

        world.system<const WorldTransform&, const MeshRenderer&, const PipelineHandleComponent&>("QueueRenderCommands")
             .kind<WorldRendererSystem>()
             .term_at(0).up<PipelineOf>()
             .term_at(1).up<PipelineOf>()
             .multi_threaded(false)
             .each([&](
                 flecs::iter& it, size_t index,
                 const WorldTransform& transform, const MeshRenderer& meshRenderer,
                 const PipelineHandleComponent& psoComponent
             ) {
                     const auto entity = it.entity(index);

                     const uint64_t sortKey =
                         (static_cast<uint64_t>(static_cast<uint32_t>(psoComponent.Order - INT32_MIN)) << 48) |
                         (static_cast<uint64_t>(psoComponent.PSO.GetHandle().Index()) << 24) |
                         static_cast<uint64_t>(meshRenderer.Mesh.GetAssetHandle().Index);

                     m_DrawCalls.emplace_back(
                         sortKey,
                         entity,
                         &meshRenderer.Mesh.Get(),
                         psoComponent.PSO.GetHandle(),
                         transform.Value,
                         psoComponent.MaterialIndex
                     );

                     m_InstancingDrawCalls.emplace_back(
                         transform.Value,
                         &meshRenderer.Mesh.Get(),
                         meshRenderer.Mesh.GetAssetHandle().Index,
                         entity.has<DepthWriteTag>()
                     );
                 }
             );


        m_PSOQuery = world.query_builder<const MeshRenderer&>()
                            .without<CheckedMeshRenderer>()
                            .build();

        m_DirectionalLightQuery = world.query<const WorldTransform&, const DirectionalLight&>();
        m_DirectionalLightCreateSMQuery = world.query_builder<const DirectionalLight&, const EntityID&>()
                                         .with<WorldTransform>()
                                         .without<DirectionalLightShadowMapTag>()
                                         .build();

        m_DirectionalLightSMQuery = world.query_builder<const WorldTransform&, const EntityID&>()
                                         .with<DirectionalLightShadowMapTag>()
                                         .build();

        m_DrawCalls.clear();
        m_InstancingDrawCalls.clear();

        m_World = &world;
    }

    void WorldRenderer::SetDepthPrepassShader(const GraphicsShader* shader) {
        GraphicsPipelineStateCreateInfo psoCI;

        psoCI.Name = "DepthPrepassPipeline";

        constexpr ShaderResourceVariableSpec vars[2] = {
            { "ViewProjection", ShaderType::Vertex, ShaderResourceVariableType::Static },
            { "Instances", ShaderType::Vertex, ShaderResourceVariableType::Mutable }
        };

        psoCI.Spec.ResourceLayout.Variables = vars;

        GraphicsPipelineSpec& gfx = psoCI.GraphicsPipeline;
        gfx.RenderPass = m_DepthPrepass.GetHandle();
        gfx.RasterizerSpec.CullMode = CullMode::Back;
        gfx.RasterizerSpec.FrontCounterClockwise = true;
        gfx.SampleSpec.Count = SampleCount::x4;
        gfx.DepthStencilSpec.DepthEnable = true;
        gfx.DepthStencilSpec.DepthWriteEnable = true;
        gfx.DepthStencilSpec.DepthEnable = true;
        gfx.DepthStencilSpec.DepthFunc = ComparisonFunc::LessEqual;

        LayoutElementBuilder<5> layoutBuilder{
            LayoutElement{0, 0, ValueType::Float3},
            LayoutElement{1, 0, ValueType::Float3},
            LayoutElement{2, 0, ValueType::Float3},
            LayoutElement{3, 0, ValueType::Float3},
            LayoutElement{4, 0, ValueType::Float2}
        };

        gfx.InputLayout.LayoutElements = layoutBuilder;

        const GraphicsShaderPipelineData& shaderPipeline = shader->GetShaderPass("ShadowDepth")->Pipelines.front();
        psoCI.VertexShader = shaderPipeline.VertexShader;
        psoCI.FragmentShader = shaderPipeline.FragmentShader;

        m_DepthPrepassPSO = Renderer::CreatePipelineState(psoCI);

        Renderer::BindStaticVariableByName(
            m_DepthPrepassPSO.GetHandle(),
            ShaderType::Vertex,
            "ViewProjection",
            m_ViewProjectionBuffer.GetHandle()
        );

        m_DepthPrepassSRB = Renderer::CreateShaderResourceBinding(m_DepthPrepassPSO.GetHandle(), true);
        Renderer::BindVariableByName(
            ShaderType::Vertex,
            m_DepthPrepassSRB.GetHandle(),
            "Instances",
            m_InstancesBufferView,
            SetShaderResourceFlag::None
        );
    }

    constexpr uint32_t k_ReductionClear[2] = {0x7F7FFFFF, 0x00000000};

    void WorldRenderer::SetDepthReductionCompute(ComputeShader* computeShader) {
        m_DepthReductionCompute = computeShader;

        GpuBufferSpec reductionOutSpec;
        reductionOutSpec.Name = "ReductionOut";
        reductionOutSpec.Size = sizeof(uint64_t);
        reductionOutSpec.Mode = GpuBufferMode::Raw;
        reductionOutSpec.BindFlags = Bind::UnorderedAccess;
        reductionOutSpec.Usage = Usage::Default;

        m_ReductionOutBuffer = Renderer::CreateBuffer(reductionOutSpec, {});

        ComputePipelineStateCreateInfo computePipelineState;
        computePipelineState.ComputeShader = computeShader->GetShader();
        computePipelineState.Spec.Type = PipelineType::Compute;

        constexpr ShaderResourceVariableSpec vars[2] = {
            {"DepthTex", ShaderType::Compute, ShaderResourceVariableType::Mutable},
            {"ReductionOut", ShaderType::Compute, ShaderResourceVariableType::Static},
        };

        computePipelineState.Spec.ResourceLayout.Variables = vars;

        m_ShadowComputePSO = Renderer::CreatePipelineState(computePipelineState);

        Renderer::BindStaticVariableByName(
            m_ShadowComputePSO.GetHandle(),
            ShaderType::Compute,
            "ReductionOut",
            Renderer::GetDefaultBufferView(m_ReductionOutBuffer.GetHandle(), BufferViewType::UnorderedAccess)
        );
    }

    void WorldRenderer::SetShadowDepthShader(const GraphicsShader* shaderDepthShader) {
        GpuBufferSpec lightViewProjUBSpec;
        lightViewProjUBSpec.Name = "ViewProjection";
        lightViewProjUBSpec.Size = sizeof(pfloat4x4);
        lightViewProjUBSpec.Mode = GpuBufferMode::Formatted;
        lightViewProjUBSpec.Usage = Usage::Default;
        lightViewProjUBSpec.BindFlags = Bind::UniformBuffer;
        lightViewProjUBSpec.ElementByteStride = sizeof(pfloat4x4);

        m_ViewProjectionBuffer = Renderer::CreateBuffer(lightViewProjUBSpec, {});

        GraphicsPipelineStateCreateInfo psoCI;

        psoCI.Name = "ShadowDepthPipeline";

        constexpr ShaderResourceVariableSpec vars[2] = {
            { "ViewProjection", ShaderType::Vertex, ShaderResourceVariableType::Static },
            { "Instances", ShaderType::Vertex, ShaderResourceVariableType::Mutable }
        };

        psoCI.Spec.ResourceLayout.Variables = vars;

        GraphicsPipelineSpec& gfx = psoCI.GraphicsPipeline;
        gfx.RenderPass = m_ShadowRenderPass;
        gfx.RasterizerSpec.CullMode = CullMode::Front;
        gfx.RasterizerSpec.DepthBias = 3;
        gfx.RasterizerSpec.SlopeScaledDepthBias = 8.0f;
        gfx.RasterizerSpec.DepthBiasClamp = 0.0f;
        //gfx.RasterizerSpec.DepthClipEnable = false; // Not enable by default? TODO: maybe check enable the feature conditionally
        gfx.DepthStencilSpec.DepthEnable = true;

        LayoutElementBuilder<5> layoutBuilder{
            LayoutElement{0, 0, ValueType::Float3},
            LayoutElement{1, 0, ValueType::Float3},
            LayoutElement{2, 0, ValueType::Float3},
            LayoutElement{3, 0, ValueType::Float3},
            LayoutElement{4, 0, ValueType::Float2}
        };

        gfx.InputLayout.LayoutElements = layoutBuilder;

        psoCI.VertexShader = shaderDepthShader->GetShaderPass("ShadowDepth")->Pipelines.front().VertexShader;

        m_ShadowDepthPSO = Renderer::CreatePipelineState(psoCI);

        Renderer::BindStaticVariableByName(
            m_ShadowDepthPSO.GetHandle(),
            ShaderType::Vertex,
            "ViewProjection",
            m_ViewProjectionBuffer.GetHandle()
        );

        m_ShadowDepthSRB = Renderer::CreateShaderResourceBinding(m_ShadowDepthPSO.GetHandle(), true);
        Renderer::BindVariableByName(
            ShaderType::Vertex,
            m_ShadowDepthSRB.GetHandle(),
            "Instances",
            m_InstancesBufferView,
            SetShaderResourceFlag::None
        );
    }

    void WorldRenderer::SetShadowMaskShader(const GraphicsShader* graphicsShader) {
        const GraphicsShaderPass* pass = graphicsShader->GetShaderPass("ShadowMask");

        GraphicsPipelineStateCreateInfo psoCI{};
        psoCI.Name = "ShadowMask";
        psoCI.GraphicsPipeline.RenderPass = m_ShadowMaskRenderPass.GetHandle();

        psoCI.VertexShader = pass->Pipelines.front().VertexShader;
        psoCI.FragmentShader = pass->Pipelines.front().FragmentShader;

        // No input layout, vertex shader generates positions
        psoCI.GraphicsPipeline.InputLayout.LayoutElements = {};

        psoCI.GraphicsPipeline.RasterizerSpec.CullMode = CullMode::None; // no culling on fullscreen tri

        // No depth
        psoCI.GraphicsPipeline.DepthStencilSpec.DepthEnable = false;

        constexpr ShaderResourceVariableSpec vars[4] = {
            {"CascadeData", ShaderType::Fragment, ShaderResourceVariableType::Static},
            {"SceneDepth", ShaderType::Fragment, ShaderResourceVariableType::Mutable},
            {"SceneNormal", ShaderType::Fragment, ShaderResourceVariableType::Mutable},
            {"ShadowMaps", ShaderType::Fragment, ShaderResourceVariableType::Mutable},
        };

        psoCI.Spec.ResourceLayout.Variables = vars;

        SamplerSpec samplerSpec;
        samplerSpec.WrapU = WrapMode::Clamp;
        samplerSpec.WrapV = WrapMode::Clamp;
        samplerSpec.MinFilter = FilterMode::ComparisonLinear;
        samplerSpec.MagFilter = FilterMode::ComparisonLinear;
        samplerSpec.MipFilter = FilterMode::ComparisonLinear;

        ImmutableSamplerSpec samplers[1];
        samplers[0].SamplerOrTextureName = "ShadowMaps";
        samplers[0].Specification = samplerSpec;
        samplers[0].ShaderStages = ShaderType::Fragment;
        samplers[0].Specification.ComparisonFunc = ComparisonFunc::LessEqual;

        psoCI.Spec.ResourceLayout.ImmutableSamplers = samplers;

        m_ShadowMaskPSO = Renderer::CreatePipelineState(psoCI);

        Renderer::BindStaticVariableByName(
            m_ShadowMaskPSO.GetHandle(),
            ShaderType::Fragment,
            "CascadeData",
            m_CascadeShadowDataBuffer.GetHandle()
        );
    }

    void WorldRenderer::CreatePerViewResources(
        const Scope<WorldRendererView>& view,
        const MaterialRegistry& materialRegistry,
        const WeakPipelineData& pipeline
    ) const {
        ResourceRef<ShaderResourceBinding>* handle = nullptr;

        const auto it = view->ViewSRBs.find(pipeline.PSO);
        if (it != view->ViewSRBs.end()) {
            if (it->second.IsValid()) {
                Renderer::Destroy(it->second.GetHandle());
            }

            it->second = {};
            handle = &it->second;
        } else {
            handle = &view->ViewSRBs.try_emplace(pipeline.PSO).first->second;
        }

        const ShaderResourceBindingHandle srb = Renderer::CreateShaderResourceBinding(pipeline.PSO, true);
        Renderer::BindVariableByName(ShaderType::Vertex, srb, "Instances", m_InstancesBufferView, SetShaderResourceFlag::None);
        Renderer::BindVariableByName(ShaderType::Fragment, srb, "Instances", m_InstancesBufferView, SetShaderResourceFlag::None);

        if (pipeline.HasShadowMask) {
            Renderer::BindVariableByName(
               ShaderType::Fragment,
               srb,
               "ShadowMask",
               Renderer::TextureGetDefaultView(view->ShadowMask.GetHandle(), TextureViewType::ShaderResource),
               SetShaderResourceFlag::None
           );
        }

        Renderer::BindVariableByName(
            ShaderType::Vertex,
            srb,
            "Materials",
            materialRegistry.GetBufferViewHandle(),
            SetShaderResourceFlag::AllowOverwrite
        );

        Renderer::BindVariableByName(
            ShaderType::Fragment,
            srb,
            "Materials",
            materialRegistry.GetBufferViewHandle(),
            SetShaderResourceFlag::AllowOverwrite
        );

        Renderer::BindArrayByName(
           ShaderType::Fragment,
           srb,
           "g_Textures",
           materialRegistry.GetTextureRegistry().GetTextureViews(),
           SetShaderResourceFlag::AllowOverwrite
       );

        Renderer::TransitionShaderResources(srb);

        *handle = srb;
    }

    const WorldRendererView* WorldRenderer::CreateView(std::string_view name, Extent2D size) {
        Scope<WorldRendererView> view = CreateScope<WorldRendererView>(this, static_cast<uint32_t>(m_ActiveViews.size()));

        {
            view->Size = size;
            TextureSpecification msaaColorSpec;
            msaaColorSpec.Width = size.Width;
            msaaColorSpec.Height = size.Height;

            msaaColorSpec.Format = ImageFormat::RGBA8UNorm;
            msaaColorSpec.SamplerWrap = WrapMode::Clamp;

            msaaColorSpec.BindFlags = Bind::RenderTarget;
            msaaColorSpec.SampleCount = SampleCount::x4;

            view->SceneColorMSAA = Renderer::CreateTexture(msaaColorSpec);

            TextureSpecification sceneColor;
            sceneColor.Width = size.Width;
            sceneColor.Height = size.Height;

            sceneColor.Format = ImageFormat::RGBA8UNorm;
            sceneColor.SamplerWrap = WrapMode::Repeat;

            sceneColor.BindFlags = Bind::RenderTarget | Bind::ShaderResource;
            sceneColor.SampleCount = SampleCount::x1;

            view->SceneColor = Renderer::CreateTexture(sceneColor);

            view->SceneColorRTV = Renderer::TextureGetDefaultView(view->SceneColor.GetHandle(), TextureViewType::RenderTarget);
            view->SceneColorSRV = Renderer::TextureGetDefaultView(view->SceneColor.GetHandle(), TextureViewType::ShaderResource);

            TextureSpecification msaaDepthSpec;
            msaaDepthSpec.Width  = size.Width;
            msaaDepthSpec.Height = size.Height;

            msaaDepthSpec.Format = ImageFormat::Depth32Float;
            msaaDepthSpec.SamplerWrap = WrapMode::Repeat;

            msaaDepthSpec.BindFlags = Bind::DepthStencil | Bind::ShaderResource;
            msaaDepthSpec.SampleCount = SampleCount::x4;

            view->SceneDepthMSAA = Renderer::CreateTexture(msaaDepthSpec);
            view->SceneDepthSRV = Renderer::TextureGetDefaultView(
                view->SceneDepthMSAA.GetHandle(),
                TextureViewType::ShaderResource
            );

            view->SceneDepthDSV = Renderer::TextureGetDefaultView(
                view->SceneDepthMSAA.GetHandle(),
                TextureViewType::DepthStencil
            );


            TextureSpecification sceneNormal;
            sceneNormal.Width = size.Width;
            sceneNormal.Height = size.Height;
            sceneNormal.Format = ImageFormat::RGBA16Float;
            sceneNormal.SamplerWrap = WrapMode::Repeat;
            sceneNormal.BindFlags = Bind::RenderTarget | Bind::ShaderResource;
            sceneNormal.SampleCount = SampleCount::x4;

            view->SceneNormalMSAA = Renderer::CreateTexture(sceneNormal);

            view->SceneNormalRTV = Renderer::TextureGetDefaultView(
                view->SceneNormalMSAA.GetHandle(),
                TextureViewType::RenderTarget
            );
            view->SceneNormalSRV = Renderer::TextureGetDefaultView(
                view->SceneNormalMSAA.GetHandle(),
                TextureViewType::ShaderResource
            );
        }

        {
            const TextureViewHandle attachments[] = {
                Renderer::TextureGetDefaultView(view->SceneColorMSAA.GetHandle(), TextureViewType::RenderTarget),
                view->SceneColorRTV,
                view->SceneDepthDSV
            };

            FrameBufferSpec spec;
            spec.Attachments = attachments;
            spec.Name = name;
            spec.RenderPassHandle = m_RenderPass;
            spec.Size = size;

            view->SceneFB = Renderer::CreateFrameBuffer(spec);
        }

        {
            static std::string depthPrepassName;
            depthPrepassName = FormatTemp("{}_DepthPrepass", name);

            FrameBufferSpec depthFBSpec;
            depthFBSpec.Name = depthPrepassName;
            depthFBSpec.RenderPassHandle = m_DepthPrepass.GetHandle();

            TextureViewHandle attachments[2] = { view->SceneDepthDSV, view->SceneNormalRTV };

            depthFBSpec.Attachments = attachments;
            depthFBSpec.Size = size;

            view->DepthPrepassFB = Renderer::CreateFrameBuffer(depthFBSpec);
        }

        {
            TextureSpecification shadowMaskSpec;
            shadowMaskSpec.Type = TextureType::Texture2D;
            shadowMaskSpec.Format = ImageFormat::R8UNorm;
            shadowMaskSpec.Width = size.Width;
            shadowMaskSpec.Height = size.Height;
            shadowMaskSpec.BindFlags = Bind::RenderTarget | Bind::ShaderResource;

            view->ShadowMask = Renderer::CreateTexture(shadowMaskSpec);

            Renderer::TransitionResource(view->ShadowMask.GetHandle(), ResourceState::ShaderResource);

            TextureViewHandle maskRTV = Renderer::TextureGetDefaultView(
                view->ShadowMask.GetHandle(),
                TextureViewType::RenderTarget
            );

            FrameBufferSpec shadowMaskFBSpec;
            shadowMaskFBSpec.Name = "ShadowMaskFB";
            shadowMaskFBSpec.RenderPassHandle = m_ShadowMaskRenderPass.GetHandle();
            shadowMaskFBSpec.Attachments = Span32(&maskRTV, 1);
            shadowMaskFBSpec.Size = size;

            view->ShadowMaskFB = Renderer::CreateFrameBuffer(shadowMaskFBSpec);

            view->ShadowMaskSRB = Renderer::CreateShaderResourceBinding(m_ShadowMaskPSO.GetHandle(), true);

            Renderer::BindVariableByName(
                ShaderType::Fragment,
                view->ShadowMaskSRB.GetHandle(),
                "SceneDepth",
                view->SceneDepthSRV,
                SetShaderResourceFlag::None
            );

            Renderer::BindVariableByName(
                ShaderType::Fragment,
                view->ShadowMaskSRB.GetHandle(),
                "SceneNormal",
                view->SceneNormalSRV,
                SetShaderResourceFlag::None
            );
        }

        {
            GpuBufferSpec stagingSpec;
            stagingSpec.Name = "ReductionStaging";
            stagingSpec.Size = sizeof(uint64_t);
            stagingSpec.Mode = GpuBufferMode::Raw;
            stagingSpec.BindFlags = Bind::None;
            stagingSpec.Usage = Usage::Staging;
            stagingSpec.CpuAccessFlags = CpuAccess::Read;

            view->ReductionStagingBuffer = Renderer::CreateBuffer(stagingSpec, {});

            view->ShadowComputeSRB = Renderer::CreateShaderResourceBinding(m_ShadowComputePSO.GetHandle(), true);

            Renderer::BindVariableByName(
                ShaderType::Compute,
                view->ShadowComputeSRB.GetHandle(),
                "DepthTex",
                view->SceneDepthSRV,
                SetShaderResourceFlag::None
            );
        }

        for (const auto& pipelineInfo : m_PipelineStates | std::views::values) {
            for (const auto& pipeline : pipelineInfo.Pipelines) {
                CreatePerViewResources(view, pipelineInfo.MaterialRegistry, pipeline);
            }
        }

        m_ActiveViews.push_back(std::move(view));
        return m_ActiveViews.back().get();
    }

    void WorldRenderer::ResizeView(const WorldRendererView* worldRendererView, const Extent2D size) const {
        QS_CORE_ASSERT(worldRendererView, "[WorldRenderer] WorldRenderer::ResizeView requires a valid WorldRendererView!");

        auto& view = m_ActiveViews[worldRendererView->GetViewID()];

        view->Size = size;

        Renderer::TextureResize(view->SceneColorMSAA.GetHandle(), size.Width, size.Height);
        Renderer::TextureResize(view->SceneColor.GetHandle(), size.Width, size.Height);
        Renderer::TextureResize(view->SceneDepthMSAA.GetHandle(), size.Width, size.Height);
        Renderer::TextureResize(view->SceneNormalMSAA.GetHandle(), size.Width, size.Height);
        Renderer::FrameBufferResize(view->SceneFB.GetHandle(), size.Width, size.Height);
        Renderer::FrameBufferResize(view->DepthPrepassFB.GetHandle(), size.Width, size.Height);

        Renderer::TextureResize(view->ShadowMask.GetHandle(), size.Width, size.Height);
        Renderer::FrameBufferResize(view->ShadowMaskFB.GetHandle(), size.Width, size.Height);

        view->ShadowComputeSRB = Renderer::CreateShaderResourceBinding(m_ShadowComputePSO.GetHandle(), true);

        Renderer::BindVariableByName(
            ShaderType::Compute,
            view->ShadowComputeSRB.GetHandle(),
            "DepthTex",
            view->SceneDepthSRV,
            SetShaderResourceFlag::None
        );

        view->ShadowMaskSRB = Renderer::CreateShaderResourceBinding(m_ShadowMaskPSO.GetHandle(), true);
        view->ShadowMapsBound = false;

        Renderer::BindVariableByName(
            ShaderType::Fragment,
            view->ShadowMaskSRB.GetHandle(),
            "SceneDepth",
            view->SceneDepthSRV,
            SetShaderResourceFlag::None
        );

        Renderer::BindVariableByName(
            ShaderType::Fragment,
            view->ShadowMaskSRB.GetHandle(),
            "SceneNormal",
            view->SceneNormalSRV,
            SetShaderResourceFlag::None
        );

        for (const auto& pipeline : m_PipelineStates | std::views::values) {
            for (const auto & weakPipelineData : pipeline.Pipelines) {
                CreatePerViewResources(view, pipeline.MaterialRegistry, weakPipelineData);
            }
        }
    }

    void WorldRenderer::Begin() {
        QS_CORE_ASSERT(m_World, "WorldRenderer `m_World` is nullptr!");
        bool isDirty = false;

        m_World->defer_begin();
        m_DirectionalLightCreateSMQuery.each([&](const flecs::entity entity, const DirectionalLight&, const EntityID& entityID) {
            auto it = m_ShadowMaps.find(entityID);
            if (it != m_ShadowMaps.end()) {
                entity.add<DirectionalLightShadowMapTag>();
                return;
            }

            TextureSpecification spec;
            spec.Type = TextureType::Texture2DArray;
            spec.Format = ImageFormat::Depth32Float;
            spec.Width = spec.Height = 2048;
            spec.ArraySize = k_NumCascades;
            spec.SampleCount = SampleCount::x1;
            spec.MipLevels = 1;
            spec.BindFlags = Bind::DepthStencil | Bind::ShaderResource;

            TextureHandle shadowMap = Renderer::CreateTexture(spec);

            Array<ResourceRef<FrameBuffer>, k_NumCascades> shadowFrameBuffers;

            for (uint32_t i = 0; i < k_NumCascades; i++) {
                TextureViewSpec dsvSpec;
                dsvSpec.ViewType = TextureViewType::DepthStencil;
                dsvSpec.TextureType = TextureType::Texture2DArray;
                dsvSpec.Format = ImageFormat::Depth32Float;
                dsvSpec.FirstArraySlice = i;
                dsvSpec.NumArraySlices = 1;
                dsvSpec.MostDetailedMip = 0;
                dsvSpec.NumMipLevels = 1;

                TextureViewHandle dsv = Renderer::TextureCreateView(shadowMap, dsvSpec);

                FrameBufferSpec fbSpec;
                thread_local std::string name;
                name = FormatTemp("CascadeShadowMap_{}", i);
                fbSpec.Name = name;
                fbSpec.Size = { 2048, 2048 };
                fbSpec.Attachments = Span32{ &dsv, 1 };
                fbSpec.RenderPassHandle = m_ShadowRenderPass;
                fbSpec.NumArraySlices = 1;

                shadowFrameBuffers[i] = Renderer::CreateFrameBuffer(fbSpec);
            }

            m_ShadowMaps.try_emplace(entityID, shadowMap, std::move(shadowFrameBuffers));
        });

        m_World->defer_end();

        m_World->defer_begin();

        m_RenderingQuery.each([&](
            const flecs::entity entity, const WorldTransform&, const MeshRenderer& meshRenderer,
            const PipelineHandleComponent& pipelineStateComponent
        ) {
            if (!meshRenderer.Mesh
                || !meshRenderer.Material
                || meshRenderer.Material->GetShader().GetAssetID() != pipelineStateComponent.ShaderID
            ) {
                entity.target<PipelineOf>().remove<CheckedMeshRenderer>();
                entity.destruct();
                isDirty = true;
            }

            if (!Renderer::IsAlive(pipelineStateComponent.PSO.GetHandle())) {
                m_PipelineStates.erase(pipelineStateComponent.ShaderID);
                for (const auto& worldRendererView : m_ActiveViews) {
                    worldRendererView->ViewSRBs.erase(pipelineStateComponent.PSO.GetHandle());
                }
                entity.target<PipelineOf>().remove<CheckedMeshRenderer>();
                entity.destruct();
            }
        });

        m_World->defer_end();

        m_World->defer_begin();

        m_PSOQuery.each([&](flecs::entity entity, const MeshRenderer& meshRenderer) {
            if (!meshRenderer.Mesh || !meshRenderer.Material) {
                return;
            }

            Material& material = meshRenderer.Material.Get();
            if (!material.GetShader()) {
                return;
            }

            isDirty = true;

            GraphicsShader& shader = material.GetShader().Get();
            const auto it = m_PipelineStates.find(shader.GetAssetID());
            if (it != m_PipelineStates.end()) {
                if (Renderer::IsAlive(it->second.Pipelines.front().PSO)) {
                    for (const WeakPipelineData& pipeline : it->second.Pipelines) {
                        flecs::entity pipelineHandle = m_World->entity()
                              .add<PipelineOf>(entity)
                              .emplace<PipelineHandleComponent>(
                                  pipeline.PSO,
                                  pipeline.Order,
                                  it->second.MaterialRegistry.Add(meshRenderer.Material),
                                  shader.GetAssetID()
                              );

                        if (pipeline.DepthWrite) {
                            pipelineHandle.add<DepthWriteTag>();
                        }
                    }

                    entity.add<CheckedMeshRenderer>();
                    return;
                }

                m_PipelineStates.erase(it);
            }

            const GraphicsShaderPass* shaderPass = shader.GetShaderPass("GBuffer");
            if (!shaderPass) {
                return;
            }

            Vec<WeakPipelineData> worldPipelines;

            bool hasShadowMaps = false;
            for (const GraphicsShaderPipelineData& pipelineData : shaderPass->Pipelines) {
                GraphicsPipelineStateCreateInfo pipelineStateCreateInfo;

                pipelineStateCreateInfo.Name = shader.GetName();

                pipelineStateCreateInfo.VertexShader = pipelineData.VertexShader;
                pipelineStateCreateInfo.FragmentShader = pipelineData.FragmentShader;

                pipelineStateCreateInfo.GraphicsPipeline.RenderPass = m_RenderPass;
                pipelineStateCreateInfo.GraphicsPipeline.RasterizerSpec.CullMode = CullMode::Back;
                pipelineStateCreateInfo.GraphicsPipeline.RasterizerSpec.FrontCounterClockwise = true;
                pipelineStateCreateInfo.GraphicsPipeline.DepthStencilSpec.DepthEnable = true;
                pipelineStateCreateInfo.GraphicsPipeline.DepthStencilSpec.DepthWriteEnable = false;
                pipelineStateCreateInfo.GraphicsPipeline.DepthStencilSpec.DepthFunc = ComparisonFunc::LessEqual;

                bool depthWrite = true;
                for (const auto & pipelineOption : pipelineData.PipelineOptions) {
                    switch (pipelineOption.first) {
                    case PipelineOption::None:
                        break;
                    case PipelineOption::DepthEnable:
                        pipelineStateCreateInfo.GraphicsPipeline.DepthStencilSpec.DepthEnable =
                            static_cast<bool>(std::get<int32_t>(pipelineOption.second));
                        break;
                    case PipelineOption::DepthWriteEnable:
                        depthWrite = static_cast<bool>(std::get<int32_t>(pipelineOption.second));
                    break;
                    case PipelineOption::CullMode:
                        pipelineStateCreateInfo.GraphicsPipeline.RasterizerSpec.CullMode =
                            static_cast<CullMode>(std::get<int32_t>(pipelineOption.second));
                    break;
                    }
                }

                pipelineStateCreateInfo.GraphicsPipeline.SampleSpec.Count = SampleCount::x4;

                LayoutElementBuilder<5> layoutBuilder{
                    LayoutElement{0, 0, ValueType::Float3},
                    LayoutElement{1, 0, ValueType::Float3},
                    LayoutElement{2, 0, ValueType::Float3},
                    LayoutElement{3, 0, ValueType::Float3},
                    LayoutElement{4, 0, ValueType::Float2}
                };

                pipelineStateCreateInfo.GraphicsPipeline.InputLayout.LayoutElements = layoutBuilder;

                SmallVec<ShaderResourceVariableSpec, 5> vars = {
                    {"global", ShaderType::VertexAndFragment, ShaderResourceVariableType::Static},
                    {"Instances", ShaderType::VertexAndFragment, ShaderResourceVariableType::Mutable},
                };

                const uint64_t materialSize = shader.GetMaterialSize();

                if (materialSize > 0) {
                    vars.emplace_back("Materials", ShaderType::VertexAndFragment, ShaderResourceVariableType::Mutable);
                }

                vars.emplace_back("g_Textures", ShaderType::Fragment, ShaderResourceVariableType::Mutable);

                SamplerSpec samplerSpec;
                samplerSpec.WrapU = WrapMode::Repeat;
                samplerSpec.WrapV = WrapMode::Repeat;
                samplerSpec.MinFilter = FilterMode::Linear;
                samplerSpec.MagFilter = FilterMode::Linear;
                samplerSpec.MipFilter = FilterMode::Linear;

                SmallVec<ImmutableSamplerSpec, 2> immutableSamplers;
                immutableSamplers.emplace_back();
                immutableSamplers[0].SamplerOrTextureName = "g_Textures";
                immutableSamplers[0].Specification = samplerSpec;
                immutableSamplers[0].ShaderStages = ShaderType::Fragment;

                if (std::ranges::contains(shaderPass->Variables, "ShadowMask")) {
                    vars.emplace_back("ShadowMask", ShaderType::Fragment, ShaderResourceVariableType::Mutable);

                    samplerSpec.MinFilter = FilterMode::Linear;
                    samplerSpec.MagFilter = FilterMode::Linear;
                    samplerSpec.MipFilter = FilterMode::Linear;

                    immutableSamplers.emplace_back();
                    immutableSamplers[1].SamplerOrTextureName = "ShadowMask";
                    immutableSamplers[1].Specification = samplerSpec;
                    immutableSamplers[1].ShaderStages = ShaderType::Fragment;
                    hasShadowMaps = true;
                }

                pipelineStateCreateInfo.Spec.ResourceLayout.Variables = vars;
                pipelineStateCreateInfo.Spec.ResourceLayout.ImmutableSamplers = immutableSamplers;

                PipelineStateHandle pipelineStateHandle = Renderer::CreatePipelineState(pipelineStateCreateInfo);

                Renderer::BindStaticVariableByName(pipelineStateHandle, ShaderType::Vertex, "global", m_GlobalBuffer);

                worldPipelines.emplace_back(
                    pipelineStateHandle,
                    pipelineData.Order,
                    true,
                    depthWrite,
                    hasShadowMaps,
                    false
                );
                shader.AddPipelineState(pipelineStateHandle);
            }

            PipelineInfo& pipelineInfo = m_PipelineStates.try_emplace(
                shader.GetAssetID(),
                std::move(worldPipelines),
                MaterialRegistry(shader.GetName(), shader.GetMaterialSize())
            ).first->second;

            MaterialRegistry& materialRegistry = pipelineInfo.MaterialRegistry;
            materialRegistry.GetTextureRegistry().Init(m_WhiteTexture, m_MagentaTexture);
            uint32_t materialId = materialRegistry.Add(meshRenderer.Material);

            for (WeakPipelineData& weakPipelineData : pipelineInfo.Pipelines) {
                flecs::entity pipelineHandle = m_World->entity()
                      .add<PipelineOf>(entity)
                      .emplace<PipelineHandleComponent>(
                          weakPipelineData.PSO,
                          weakPipelineData.Order,
                          materialId,
                          shader.GetAssetID()
                      );

                if (weakPipelineData.DepthWrite) {
                    pipelineHandle.add<DepthWriteTag>();
                }
            }

            entity.add<CheckedMeshRenderer>();

            for (auto& view : m_ActiveViews) {
                for (const WeakPipelineData& pipeline : worldPipelines) {
                    CreatePerViewResources(view, materialRegistry, pipeline);
                }
            }
        });

        m_World->defer_end();

        m_DrawCalls.clear();
        m_InstancingDrawCalls.clear();
        m_DrawCalls.reserve(m_RenderingQuery.count());
        m_InstancingDrawCalls.reserve(m_RenderingQuery.count());

        m_World->run_pipeline(m_WorldRendererPipeline);

        for (auto& pipelineInfo : m_PipelineStates | std::views::values) {
            pipelineInfo.MaterialRegistry.FlushToGPU();
            if (pipelineInfo.MaterialRegistry.WasReallocated()) {
                for (const auto& worldRendererView : m_ActiveViews) {
                    for (const auto& pipeline : pipelineInfo.Pipelines) {
                        if (pipeline.HasTextures) {
                            CreatePerViewResources(worldRendererView, pipelineInfo.MaterialRegistry, pipeline);
                        }
                    }
                }
            }

            TextureRegistry& textureRegistry = pipelineInfo.MaterialRegistry.GetTextureRegistry();
            if (textureRegistry.IsDirty()) {
                textureRegistry.UpdateTexturesArray();

                bool failed = false;
                for (const auto& worldRendererView : m_ActiveViews) {
                    for (const auto& pipeline : pipelineInfo.Pipelines) {
                        if (pipeline.HasTextures) {
                            CreatePerViewResources(worldRendererView, pipelineInfo.MaterialRegistry, pipeline);
                            auto it = worldRendererView->ViewSRBs.find(pipeline.PSO);
                            if (it == worldRendererView->ViewSRBs.end()) {
                                failed = true;
                                continue;
                            }

                            const auto srb = it->second.GetHandle();

                            Renderer::CommitShaderResources(
                                srb,
                                ResourceStateTransitionMode::Transition
                            );
                        }
                    }
                }

                if (!failed) {
                    textureRegistry.SetDirty(false);
                }
            }
        }

        std::ranges::sort(m_DrawCalls, {}, &DrawCommand::SortKey);
        std::ranges::sort(m_InstancingDrawCalls, {}, &InstanceDrawCommand::SortKey);
    }

    void WorldRenderer::Render(const WorldRendererView* worldRendererView, const RenderViewParams& renderViewParams) const {
        QS_CORE_ASSERT(worldRendererView, "WorldRenderer WorldRenderer::Render Requires a valid WorldRendererView!");

        auto& view = *m_ActiveViews[worldRendererView->GetViewID()];

        if (view.ReductionReadbackReady) {
            void* mapped;
            Renderer::Map(view.ReductionStagingBuffer.GetHandle(), MapType::Read, MapFlags::DoNotWait, mapped);
            if (mapped) {
                auto readback = static_cast<uint32_t*>(mapped);
                view.LastMinNDC = std::bit_cast<float>(readback[0]);
                view.LastMaxNDC = std::bit_cast<float>(readback[1]);
                Renderer::Unmap(view.ReductionStagingBuffer.GetHandle(), MapType::Read);
            }
        }

        float4x4 viewProjection = math::mul(renderViewParams.View, renderViewParams.Projection);
        float4x4 invViewProj = math::inverse(viewProjection);
        float3 lightDirection(0, -1.f, 0);

        Globals globals{};
        globals.ViewProjection = viewProjection;
        globals.LightDirection = float4(lightDirection, 0);

        Renderer::UpdateBuffer(
            m_GlobalBuffer,
            0,
            std::as_bytes(Span(&globals, 1))
        );

        // Depth Prepass
        {
            Renderer::UpdateBuffer(
                m_ViewProjectionBuffer.GetHandle(),
                0,
                std::as_bytes(Span(&viewProjection, 1))
            );

            Renderer::BindPipelineState(m_DepthPrepassPSO.GetHandle());

            Renderer::CommitShaderResources(m_DepthPrepassSRB.GetHandle(), ResourceStateTransitionMode::Transition);

            BeginRenderPassAttribs depthPrepassAttribs;
            depthPrepassAttribs.FrameBufferHandle = view.DepthPrepassFB.GetHandle();
            depthPrepassAttribs.RenderPassHandle = m_DepthPrepass.GetHandle();

            ClearValue clear[2];
            clear[0].DepthStencil.Depth = 1.0f;
            clear[1].Color = { 0.5f, 0.5f, 0.5f, 0.0f };

            depthPrepassAttribs.ClearColors = clear;

            Renderer::BeginRenderPass(depthPrepassAttribs);

            uint32_t drawIndex = 0;
            while (drawIndex < m_InstancingDrawCalls.size()) {
                uint32_t drawStart = drawIndex;
                Mesh* mesh = m_InstancingDrawCalls[drawStart].Mesh;

                void* mapped;
                Renderer::Map(m_InstancesGpuBuffer, MapType::Write, MapFlags::Discard, mapped);
                auto* instances = static_cast<InstanceData*>(mapped);

                uint32_t instanceCount = 0;
                while (
                    drawIndex < m_InstancingDrawCalls.size()
                    && instanceCount < k_MaxInstances
                    && mesh == m_InstancingDrawCalls[drawIndex].Mesh
                ) {
                    if (!m_InstancingDrawCalls[drawIndex].DepthWrite) {
                        drawIndex++;
                        continue;
                    }

                    instances[instanceCount++] = InstanceData{
                        .Transform = m_InstancingDrawCalls[drawIndex].Transform,
                        .MaterialId = 0,
                        .EntityIndex = drawIndex + 1
                    };

                    drawIndex++;
                }

                Renderer::Unmap(m_InstancesGpuBuffer, MapType::Write);

                if (instanceCount == 0) {
                    continue;
                }

                Renderer::BindVertexBuffer(mesh->GetVertexBuffer(), 0);
                Renderer::BindIndexBuffer(mesh->GetIndexBuffer());

                DrawIndexedAttribs attribs;
                attribs.Flags = DrawFlags::VerifyAll;
                attribs.IndexType = ValueType::UInt16;
                attribs.NumIndices = mesh->GetIndices().size();
                attribs.NumInstances = instanceCount;
                attribs.FirstInstanceLocation = 0;

                Renderer::DrawIndexed(attribs);
            }

            Renderer::EndRenderPass();
        }

        m_DirectionalLightSMQuery.each([&](const WorldTransform& transform, const EntityID& entityId) {
            const DirectionalLightShadowMap& shadowMap = m_ShadowMaps.at(entityId);
            lightDirection = transform.Value[2].xyz;

            float3 up = fabsf(math::dot(lightDirection, float3(0, 1, 0))) > 0.99f
                            ? float3(0, 0, 1)
                            : float3(0, 1, 0);
            float4x4 lightView = float4x4::look_at(float3(0, 0, 0), lightDirection, up);

            float nearZ = renderViewParams.NearClip;
            float farZ = renderViewParams.FarClip;

            CascadeShadowData shadowData{};

            // Depth Reduction Compute (Uses depth of frame N - 1)
            float minNDC = view.ReductionReadbackReady ? view.LastMinNDC : 0.0f;
            float maxNDC = view.ReductionReadbackReady ? view.LastMaxNDC : 1.0f;

            // Linearize (Vulkan depth [0,1])
            auto linearize = [&](const float d) {
                return nearZ * farZ / (farZ - d * (farZ - nearZ));
            };
            float minView = linearize(minNDC);
            float maxView = linearize(maxNDC);

            // Logarithmic splits within tight range
            float ratio = maxView / minView;
            float4 targetSplits;
            for (int c = 0; c < k_NumCascades; c++) {
                constexpr float lambda = 0.75f;

                float p = (c + 1.0f) / static_cast<float>(k_NumCascades);
                float1 logSplit = minView * powf(ratio, p);
                float1 linSplit = minView + (maxView - minView) * p;
                targetSplits[c] = math::lerp(linSplit, logSplit, lambda);
            }

            // Temporal smoothing, prevents split jumping
            view.SmoothedSplits = targetSplits;//math::lerp(smoothedSplits, targetSplits, 0.15f);

            Array<float3, 8> frustumCorners = {
                // near plane (z=0)
                float3{-1, -1, 0}, { 1, -1, 0}, { 1,  1, 0}, {-1,  1, 0},
                // far plane (z=1)
                {-1, -1, 1}, { 1, -1, 1}, { 1,  1, 1}, {-1,  1, 1},
            };

            auto transformPoint = [](const float3& point, const float4x4& v) -> float3 {
                const float4 p = mul(float4(point, 1.0f), v);
                return p.xyz / p.w;  // perspective divide
            };

            for (auto& corner : frustumCorners) {
                corner = transformPoint(corner, invViewProj); // world space
            }

            for (int c = 0; c < k_NumCascades; c++) {
                float splitNear = c == 0 ? nearZ : view.SmoothedSplits[c - 1];
                float splitFar  = view.SmoothedSplits[c];

                Array<float3, 8> corners = frustumCorners;

                for (int i = 0; i < 4; i++) {
                    float3 ray = corners[i + 4] - corners[i];
                    corners[i + 4] = corners[i] + ray * (splitFar / farZ);
                    corners[i] = corners[i] + ray * (splitNear / farZ);
                }

                // compute light-space AABB from these 8 corners


                // Transform corners to light space, compute AABB
                float3 lsMin(math::f_max), lsMax(-math::f_max);
                for (auto& corner : corners) {
                    float3 ls = transformPoint(corner, lightView);
                    lsMin = math::min(lsMin, ls);
                    lsMax = math::max(lsMax, ls);
                }

                constexpr uint32_t SHADOW_MAP_SIZE = 2048;
                float unitsPerTexelX = (lsMax.x - lsMin.x) / static_cast<float>(SHADOW_MAP_SIZE);
                float unitsPerTexelY = (lsMax.y - lsMin.y) / static_cast<float>(SHADOW_MAP_SIZE);
                lsMin.x = math::floor(lsMin.x / unitsPerTexelX) * unitsPerTexelX;
                lsMin.y = math::floor(lsMin.y / unitsPerTexelY) * unitsPerTexelY;
                lsMax.x = lsMin.x + unitsPerTexelX * SHADOW_MAP_SIZE;
                lsMax.y = lsMin.y + unitsPerTexelY * SHADOW_MAP_SIZE;

                // Pull near plane back to catch shadow casters behind camera
                lsMin.z -= 50.0f; // needs to be tuned to scene scale, might add a UI slider

                float4x4 lightProj = mathExt::orthographic(lsMin.x, lsMax.x, lsMin.y, lsMax.y, lsMin.z, lsMax.z);
                shadowData.LightViewProj[c] = math::mul(lightView, lightProj);
            }

            Renderer::BindPipelineState(m_ShadowDepthPSO.GetHandle());
            for (uint32_t c = 0; c < k_NumCascades; c++) {
                pfloat4x4 lightViewProjection = shadowData.LightViewProj[c];
                Renderer::UpdateBuffer(
                    m_ViewProjectionBuffer.GetHandle(),
                    0,
                    std::as_bytes(Span(&lightViewProjection, 1))
                );

                Renderer::CommitShaderResources(m_ShadowDepthSRB.GetHandle(), ResourceStateTransitionMode::Transition);

                BeginRenderPassAttribs passAttribs;
                passAttribs.FrameBufferHandle = shadowMap.ShadowFrameBuffers[c].GetHandle();
                passAttribs.RenderPassHandle = m_ShadowRenderPass;

                ClearValue clear[1];
                clear->DepthStencil.Depth = 1.0f;
                passAttribs.ClearColors = clear;

                Renderer::BeginRenderPass(passAttribs);

                uint32_t drawIndex = 0;
                while (drawIndex < m_InstancingDrawCalls.size()) {
                    uint32_t drawStart = drawIndex;
                    Mesh* mesh = m_InstancingDrawCalls[drawStart].Mesh;

                    void* mapped;
                    Renderer::Map(m_InstancesGpuBuffer, MapType::Write, MapFlags::Discard, mapped);
                    auto* instances = static_cast<InstanceData*>(mapped);

                    uint32_t instanceCount = 0;
                    while (
                        drawIndex < m_InstancingDrawCalls.size()
                        && instanceCount < k_MaxInstances
                        && mesh == m_InstancingDrawCalls[drawIndex].Mesh
                    ) {
                        instances[instanceCount++] = InstanceData {
                            .Transform = m_InstancingDrawCalls[drawIndex].Transform,
                            .MaterialId = 0,
                            .EntityIndex = drawIndex + 1
                        };

                        drawIndex++;
                    }

                    Renderer::Unmap(m_InstancesGpuBuffer, MapType::Write);

                    Renderer::BindVertexBuffer(mesh->GetVertexBuffer(), 0);
                    Renderer::BindIndexBuffer(mesh->GetIndexBuffer());

                    DrawIndexedAttribs attribs;
                    attribs.Flags = DrawFlags::VerifyAll;
                    attribs.IndexType = ValueType::UInt16;
                    attribs.NumIndices = mesh->GetIndices().size();
                    attribs.NumInstances = instanceCount;
                    attribs.FirstInstanceLocation = 0;

                    Renderer::DrawIndexed(attribs);
                }

                Renderer::EndRenderPass();
            }

            {
                shadowData.InvViewProjection = invViewProj;
                shadowData.View = renderViewParams.View;
                shadowData.SplitDepths = view.SmoothedSplits;
                shadowData.LightDirection = float4(lightDirection, 2048.0f);

                Renderer::UpdateBuffer(
                    m_CascadeShadowDataBuffer.GetHandle(),
                    0,
                    std::as_bytes(Span(&shadowData, 1))
                );

                Renderer::BindPipelineState(m_ShadowMaskPSO.GetHandle());

                if (!view.ShadowMapsBound) {
                    Renderer::BindVariableByName(
                        ShaderType::Fragment,
                        view.ShadowMaskSRB.GetHandle(),
                        "ShadowMaps",
                        Renderer::TextureGetDefaultView(shadowMap.ShadowMaps.GetHandle(), TextureViewType::ShaderResource),
                        SetShaderResourceFlag::AllowOverwrite
                    );

                    view.ShadowMapsBound = true;
                }

                Renderer::CommitShaderResources(view.ShadowMaskSRB.GetHandle(), ResourceStateTransitionMode::Transition);

                BeginRenderPassAttribs shadowMaskPassAttribs{};
                shadowMaskPassAttribs.RenderPassHandle = m_ShadowMaskRenderPass.GetHandle();
                shadowMaskPassAttribs.FrameBufferHandle = view.ShadowMaskFB.GetHandle();
                ClearValue clear[1];
                clear[0].Color = Color::White();
                shadowMaskPassAttribs.ClearColors = clear;

                // LoadOp = Load, so no clear value needed
                Renderer::BeginRenderPass(shadowMaskPassAttribs);

                // No vertex buffer, shader generates the triangle from SV_VertexID
                DrawAttribs draw{};
                draw.NumVertices = 3;
                Renderer::Draw(draw);

                Renderer::EndRenderPass();

                Renderer::TransitionResource(worldRendererView->ShadowMask.GetHandle(), ResourceState::ShaderResource);
            }
        });

        ClearValue clearValues[3];
        clearValues[0].Format = ImageFormat::RGBA8UNorm;
        clearValues[0].Color = renderViewParams.SceneColorClear;

        clearValues[1] = {};

        clearValues[2].Format = ImageFormat::Depth32Float;
        clearValues[2].DepthStencil.Depth = 1.0f;

        BeginRenderPassAttribs gBufferPassAttribs;
        gBufferPassAttribs.ClearColors = clearValues;

        gBufferPassAttribs.FrameBufferHandle = view.SceneFB.GetHandle();
        gBufferPassAttribs.RenderPassHandle = m_RenderPass;

        Renderer::BeginRenderPass(gBufferPassAttribs);

        uint32_t i = 0;
        while (i < m_DrawCalls.size()) {
            //
            // PIPELINE RANGE
            //

            const PipelineStateHandle& pipelineHandle = m_DrawCalls[i].PipelineState;

            uint32_t pipelineBegin = i;

            while (i < m_DrawCalls.size() &&
                m_DrawCalls[i].PipelineState == pipelineHandle) {
                ++i;
            }

            const uint32_t pipelineEnd = i;

            //
            // BIND PIPELINE
            //

            Renderer::BindPipelineState(pipelineHandle);

            //
            // DRAW MESH RANGES
            //

            if (auto it = worldRendererView->ViewSRBs.find(pipelineHandle); it != worldRendererView->ViewSRBs.end()) {
                Renderer::CommitShaderResources(it->second.GetHandle(), ResourceStateTransitionMode::None);
            }

            uint32_t j = pipelineBegin;
            uint32_t instanceOffset = 0;

            while (j < pipelineEnd) {
                const Mesh* mesh = m_DrawCalls[j].Mesh;

                void* mapped;
                Renderer::Map(m_InstancesGpuBuffer, MapType::Write, MapFlags::Discard, mapped);
                auto* instances = static_cast<InstanceData*>(mapped);

                uint32_t instanceCount = 0;
                while (j < pipelineEnd && instanceCount < k_MaxInstances && m_DrawCalls[j].Mesh == mesh) {
                    instances[instanceCount++] = InstanceData {
                        .Transform = m_DrawCalls[j].Transform,
                        .MaterialId = m_DrawCalls[j].MaterialIndex,
                        .EntityIndex = j + 1
                    };

                    ++j;
                }

                Renderer::Unmap(m_InstancesGpuBuffer, MapType::Write);

                Renderer::BindVertexBuffer(mesh->GetVertexBuffer(), 0);
                Renderer::BindIndexBuffer(mesh->GetIndexBuffer());

                DrawIndexedAttribs attribs;
                attribs.Flags = DrawFlags::VerifyAll;
                attribs.IndexType = ValueType::UInt16;
                attribs.NumIndices = mesh->GetIndices().size();
                attribs.NumInstances = instanceCount;
                attribs.FirstInstanceLocation = 0;

                Renderer::DrawIndexed(attribs);

                instanceOffset += instanceCount;
            }
        }

        Renderer::EndRenderPass();

        // Depth Reduction
        Renderer::UpdateBuffer(
            m_ReductionOutBuffer.GetHandle(),
            0,
            std::as_bytes(Span(k_ReductionClear))
        );

        Renderer::BindPipelineState(m_ShadowComputePSO.GetHandle());
        Renderer::CommitShaderResources(view.ShadowComputeSRB.GetHandle(), ResourceStateTransitionMode::Transition);

        const auto& groupSize = m_DepthReductionCompute->GetThreadGroupSize();
        const uint32_t groupsX = (view.Size.Width  + groupSize[0] - 1) / groupSize[0];
        const uint32_t groupsY = (view.Size.Height + groupSize[1] - 1) / groupSize[1];

        DispatchComputeAttribs dispatchComputeAttribs;
        dispatchComputeAttribs.ThreadGroupCountX = groupsX;
        dispatchComputeAttribs.ThreadGroupCountY = groupsY;
        dispatchComputeAttribs.ThreadGroupCountZ = 1;

        Renderer::DispatchCompute(dispatchComputeAttribs);

        Renderer::CopyBuffer(
            m_ReductionOutBuffer.GetHandle(),
            0,
            ResourceStateTransitionMode::Transition,
            view.ReductionStagingBuffer.GetHandle(),
            0,
            sizeof(uint64_t),
            ResourceStateTransitionMode::Transition
        );

        view.ReductionReadbackReady = true;
    }

    void WorldRenderer::End() {
    }

    WorldRenderer::~WorldRenderer() {
        for (auto& pipelineState : m_PipelineStates | std::views::values) {
            pipelineState.MaterialRegistry.Release();
        }

        Renderer::Destroy(m_RenderPass);
        Renderer::Destroy(m_WhiteTexture);
        Renderer::Destroy(m_MagentaTexture);

        Renderer::Destroy(m_ShadowRenderPass);
    }
}

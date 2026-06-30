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
        TextureRegistry& textureRegistry,
        const uint64_t materialSize
    )
        : m_PipelineName(pipelineName), m_MaterialSize(materialSize), m_TextureRegistry(&textureRegistry)
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
                        uint32_t textureId = m_TextureRegistry->GetID(*texture);

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
                        uint32_t textureId = m_TextureRegistry->GetID(*texture);

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
        attachments[2].LoadOp = AttachmentLoadOp::Clear;
        attachments[2].StoreOp = AttachmentStoreOp::Store;
        attachments[2].InitialState = ResourceState::DepthWrite;
        attachments[2].FinalState = ResourceState::DepthWrite;

        AttachmentReference colorRef = {0, ResourceState::RenderTarget};
        AttachmentReference resolveRef = {1, ResourceState::ResolveDest};
        AttachmentReference depthAttachment = {2, ResourceState::DepthWrite};

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

        m_TextureRegistry.Init(m_WhiteTexture, m_MagentaTexture);

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

        m_RenderingQuery = world.query<const WorldTransform&, const MeshRenderer&, const PipelineStateComponent&>();
        m_PSOQuery = world.query_builder<const MeshRenderer&>()
                            .without<PipelineStateComponent>()
                            .build();

        m_DirectionalLightQuery = world.query<const WorldTransform&, const DirectionalLight&>();
        m_DirectionalLightCreateSMQuery = world.query_builder<const DirectionalLight&>()
                                         .with<WorldTransform>()
                                         .without<DirectionalLightShadowMap>()
                                         .build();

        m_DirectionalLightSMQuery = world.query<const WorldTransform&, const DirectionalLightShadowMap&>();

        m_DrawCalls.clear();

        m_World = &world;
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

        GpuBufferSpec stagingSpec;
        stagingSpec.Name = "ReductionStaging";
        stagingSpec.Size = sizeof(uint64_t);
        stagingSpec.Mode = GpuBufferMode::Raw;
        stagingSpec.BindFlags = Bind::None;
        stagingSpec.Usage = Usage::Staging;
        stagingSpec.CpuAccessFlags = CpuAccess::Read;
        m_ReductionStagingBuffer = Renderer::CreateBuffer(stagingSpec, {});

        ComputePipelineStateCreateInfo computePipelineState;
        computePipelineState.ComputeShader = computeShader->GetShader();
        computePipelineState.Spec.Type = PipelineType::Compute;

        constexpr ShaderResourceVariableSpec vars[2] = {
            {"DepthTex", ShaderType::Compute, ShaderResourceVariableType::Dynamic},
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

        m_ShadowComputeSRB = Renderer::CreateShaderResourceBinding(m_ShadowComputePSO.GetHandle(), true);
    }

    void WorldRenderer::SetShadowDepthShader(const GraphicsShader* shaderDepthShader) {
        GpuBufferSpec lightViewProjUBSpec;
        lightViewProjUBSpec.Name = "LightViewProjection";
        lightViewProjUBSpec.Size = sizeof(pfloat4x4);
        lightViewProjUBSpec.Mode = GpuBufferMode::Formatted;
        lightViewProjUBSpec.Usage = Usage::Default;
        lightViewProjUBSpec.BindFlags = Bind::UniformBuffer;
        lightViewProjUBSpec.ElementByteStride = sizeof(pfloat4x4);

        m_LightViewProjectionBuffer = Renderer::CreateBuffer(lightViewProjUBSpec, {});

        GraphicsPipelineStateCreateInfo psoCI;

        psoCI.Name = "ShadowDepthPipeline";

        constexpr ShaderResourceVariableSpec vars[2] = {
            { "LightViewProjection", ShaderType::Vertex, ShaderResourceVariableType::Static },
            { "Instances", ShaderType::Vertex, ShaderResourceVariableType::Mutable }
        };

        psoCI.Spec.ResourceLayout.Variables = vars;

        GraphicsPipelineSpec& gfx = psoCI.GraphicsPipeline;
        gfx.RenderPass = m_ShadowRenderPass;
        gfx.RasterizerSpec.CullMode = CullMode::Back;
        gfx.RasterizerSpec.DepthBias = 4;
        gfx.RasterizerSpec.SlopeScaledDepthBias = 2.0f;
        gfx.RasterizerSpec.DepthBiasClamp = 0.0f;
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
            "LightViewProjection",
            m_LightViewProjectionBuffer.GetHandle()
        );

        m_ShadowDepthSRB = Renderer::CreateShaderResourceBinding(m_ShadowDepthPSO.GetHandle(), true);
        Renderer::BindVariableByName(
            ShaderType::Vertex,
            m_ShadowDepthSRB.GetHandle(),
            "Instances",
            m_InstancesBufferView
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

        constexpr ShaderResourceVariableSpec vars[3] = {
            {"CascadeData", ShaderType::Fragment, ShaderResourceVariableType::Static},
            {"SceneDepth", ShaderType::Fragment, ShaderResourceVariableType::Dynamic},
            {"ShadowMaps", ShaderType::Fragment, ShaderResourceVariableType::Dynamic},
        };

        psoCI.Spec.ResourceLayout.Variables = vars;

        SamplerSpec samplerSpec;
        samplerSpec.WrapU = WrapMode::Clamp;
        samplerSpec.WrapV = WrapMode::Clamp;
        samplerSpec.MinFilter = FilterMode::Point;
        samplerSpec.MagFilter = FilterMode::Point;
        samplerSpec.MipFilter = FilterMode::Point;

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

        m_ShadowMaskSRB = Renderer::CreateShaderResourceBinding(m_ShadowMaskPSO.GetHandle(), true);
    }

    void WorldRenderer::Begin() {
        QS_CORE_ASSERT(m_World, "WorldRenderer `m_World` is nullptr!");
        bool isDirty = false;

        m_World->defer_begin();
        m_DirectionalLightCreateSMQuery.each([&](const flecs::entity entity, const DirectionalLight&) {
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
                dsvSpec.TextureType = TextureType::Texture2D;
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

            entity.emplace<DirectionalLightShadowMap>(shadowMap, std::move(shadowFrameBuffers));
        });

        m_World->defer_end();

        m_World->defer_begin();

        m_RenderingQuery.each([&](
            const flecs::entity entity, const WorldTransform&, const MeshRenderer& meshRenderer,
            const PipelineStateComponent& pipelineStateComponent
        ) {
            if (!meshRenderer.Mesh
                || !meshRenderer.Material
                || meshRenderer.Material->GetShader().GetAssetID() != pipelineStateComponent.ShaderID
            ) {
                entity.remove<PipelineStateComponent>();
                isDirty = true;
            }

            for (const PipelineData& pipeline : pipelineStateComponent.Pipelines) {
                if (!Renderer::IsAlive(pipeline.PSO.GetHandle())) {
                    m_PipelineStates.erase(pipelineStateComponent.ShaderID);
                    entity.remove<PipelineStateComponent>();
                    return;
                }
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
                    Vec<PipelineData> pipelines;
                    pipelines.reserve(it->second.Pipelines.size());
                    std::ranges::transform(
                        it->second.Pipelines,
                        std::back_inserter(pipelines),
                        [](const WeakPipelineData& pipelineData) {
                            return PipelineData{
                                .PSO = pipelineData.PSO,
                                .SRB = pipelineData.SRB,
                                .Order = pipelineData.Order
                            };
                        }
                    );

                    entity.emplace<PipelineStateComponent>(
                        std::move(pipelines),
                        it->second.MaterialRegistry.Add(meshRenderer.Material),
                        shader.GetAssetID()
                    );

                    return;
                }

                m_PipelineStates.erase(it);
            }

            const GraphicsShaderPass* shaderPass = shader.GetShaderPass("GBuffer");
            if (!shaderPass) {
                return;
            }

            Vec<WeakPipelineData> worldPipelines;

            for (const GraphicsShaderPipelineData& pipelineData : shaderPass->Pipelines) {
                GraphicsPipelineStateCreateInfo pipelineStateCreateInfo;

                pipelineStateCreateInfo.Name = shader.GetName();

                pipelineStateCreateInfo.VertexShader = pipelineData.VertexShader;
                pipelineStateCreateInfo.FragmentShader = pipelineData.FragmentShader;

                pipelineStateCreateInfo.GraphicsPipeline.RenderPass = m_RenderPass;
                pipelineStateCreateInfo.GraphicsPipeline.RasterizerSpec.CullMode = CullMode::Back;
                pipelineStateCreateInfo.GraphicsPipeline.RasterizerSpec.FrontCounterClockwise = true;
                pipelineStateCreateInfo.GraphicsPipeline.DepthStencilSpec.DepthEnable = true;
                pipelineStateCreateInfo.GraphicsPipeline.DepthStencilSpec.DepthFunc = ComparisonFunc::LessEqual;

                for (const auto & pipelineOption : pipelineData.PipelineOptions) {
                    switch (pipelineOption.first) {
                    case PipelineOption::None:
                        break;
                    case PipelineOption::DepthEnable:
                        pipelineStateCreateInfo.GraphicsPipeline.DepthStencilSpec.DepthEnable =
                            static_cast<bool>(std::get<int32_t>(pipelineOption.second));
                        break;
                    case PipelineOption::DepthWriteEnable:
                        pipelineStateCreateInfo.GraphicsPipeline.DepthStencilSpec.DepthWriteEnable =
                            static_cast<bool>(std::get<int32_t>(pipelineOption.second));
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

                SmallVec<ShaderResourceVariableSpec, 4> vars = {
                    {"global", ShaderType::VertexAndFragment, ShaderResourceVariableType::Static},
                    {"Instances", ShaderType::VertexAndFragment, ShaderResourceVariableType::Mutable},
                };

                const uint64_t materialSize = shader.GetMaterialSize();

                if (materialSize > 0) {
                    vars.emplace_back("Materials", ShaderType::VertexAndFragment, ShaderResourceVariableType::Mutable);
                }

                vars.emplace_back("g_Textures", ShaderType::Fragment, ShaderResourceVariableType::Dynamic);

                SamplerSpec samplerSpec;
                samplerSpec.WrapU = WrapMode::Repeat;
                samplerSpec.WrapV = WrapMode::Repeat;
                samplerSpec.MinFilter = FilterMode::Linear;
                samplerSpec.MagFilter = FilterMode::Linear;
                samplerSpec.MipFilter = FilterMode::Linear;

                ImmutableSamplerSpec immutableSampler;
                immutableSampler.SamplerOrTextureName = "g_Textures";
                immutableSampler.Specification = samplerSpec;
                immutableSampler.ShaderStages = ShaderType::Fragment;

                pipelineStateCreateInfo.Spec.ResourceLayout.Variables = vars;
                pipelineStateCreateInfo.Spec.ResourceLayout.ImmutableSamplers = {&immutableSampler, 1};

                PipelineStateHandle pipelineStateHandle = Renderer::CreatePipelineState(pipelineStateCreateInfo);

                Renderer::BindStaticVariableByName(pipelineStateHandle, ShaderType::Vertex, "global", m_GlobalBuffer);

                ShaderResourceBindingHandle srb = Renderer::CreateShaderResourceBinding(pipelineStateHandle, true);
                Renderer::BindVariableByName(ShaderType::Vertex, srb, "Instances", m_InstancesBufferView);
                Renderer::BindVariableByName(ShaderType::Fragment, srb, "Instances", m_InstancesBufferView);

                worldPipelines.emplace_back(pipelineStateHandle, srb, pipelineData.Order, true);
                shader.AddPipelineState(pipelineStateHandle);
            }

            Vec<PipelineData> entityPipelines;
            entityPipelines.reserve(worldPipelines.size());
            std::ranges::transform(
                worldPipelines,
                std::back_inserter(entityPipelines),
                [](const WeakPipelineData& pipeline) {
                    return PipelineData{pipeline.PSO, pipeline.SRB, pipeline.Order};
                }
            );

            MaterialRegistry& materialRegistry = m_PipelineStates.try_emplace(
                shader.GetAssetID(),
                std::move(worldPipelines),
                MaterialRegistry(shader.GetName(), m_TextureRegistry, shader.GetMaterialSize())
            ).first->second.MaterialRegistry;

            entity.emplace<PipelineStateComponent>(
                std::move(entityPipelines),
                materialRegistry.Add(meshRenderer.Material),
                shader.GetAssetID()
            );
        });

        m_World->defer_end();

        m_DrawCalls.clear();
        m_DrawCalls.reserve(m_RenderingQuery.count());

        m_World->defer_begin();

        m_RenderingQuery.each([&](
            flecs::entity entity,
            const WorldTransform& transform, const MeshRenderer& meshRenderer,
            const PipelineStateComponent& pipelineStateComponent
        ) {
            for (const auto& pipeline : pipelineStateComponent.Pipelines) {
                uint64_t sortKey =
                    (static_cast<uint64_t>(static_cast<uint32_t>(pipeline.Order - INT32_MIN)) << 48) |
                    (static_cast<uint64_t>(pipeline.PSO.GetHandle().Index()) << 24) |
                    static_cast<uint64_t>(meshRenderer.Mesh.GetAssetHandle().Index);

                m_DrawCalls.emplace_back(
                    sortKey,
                    entity,
                    &meshRenderer.Mesh.Get(),
                    pipeline.PSO.GetHandle(),
                    pipeline.SRB.GetHandle(),
                    transform.Value,
                    pipelineStateComponent.MaterialIndex
                );
            }

            m_InstancingDrawCalls.emplace_back(transform.Value, &meshRenderer.Mesh.Get(), meshRenderer.Mesh.GetAssetHandle().Index);
        });

        m_World->defer_end();

        for (auto& pipelineInfo : m_PipelineStates | std::views::values) {
            pipelineInfo.MaterialRegistry.FlushToGPU();

            if (pipelineInfo.MaterialRegistry.WasReallocated()) {
                for (const auto& pipeline : pipelineInfo.Pipelines) {
                    Renderer::BindVariableByName(
                        ShaderType::Vertex,
                        pipeline.SRB,
                        "Materials",
                        pipelineInfo.MaterialRegistry.GetBufferViewHandle()
                    );

                    Renderer::BindVariableByName(
                        ShaderType::Fragment,
                        pipeline.SRB,
                        "Materials",
                        pipelineInfo.MaterialRegistry.GetBufferViewHandle()
                    );
                }
            }

            m_TextureRegistry.UpdateTexturesArray();

            for (const auto& pipeline : pipelineInfo.Pipelines) {
                if (pipeline.HasTextures) {
                    Renderer::BindArrayByName(
                       ShaderType::Fragment,
                       pipeline.SRB,
                       "g_Textures",
                       m_TextureRegistry.GetTextureViews()
                   );

                    Renderer::CommitShaderResources(pipeline.SRB, ResourceStateTransitionMode::Transition);
                }
            }
        }

        m_TextureRegistry.SetDirty(false);

        std::ranges::sort(m_DrawCalls, {}, &DrawCommand::SortKey);
        std::ranges::sort(m_InstancingDrawCalls, {}, &InstanceDrawCommand::SortKey);
    }

    void WorldRenderer::Render(
        const BeginRenderPassAttribs& beginRenderPassAttribs,
        const float4x4& view,
        const float4x4& projection,
        const TextureViewHandle sceneDepthSRV,
        const FrameBufferHandle shadowMaskFB
    ) {
        if (m_ReductionReadbackReady) {
            void* mapped;
            Renderer::Map(m_ReductionStagingBuffer.GetHandle(), MapType::Read, MapFlags::DoNotWait, mapped);
            if (mapped) {
                auto readback = static_cast<uint32_t*>(mapped);
                m_LastMinNDC = std::bit_cast<float>(readback[0]);
                m_LastMaxNDC = std::bit_cast<float>(readback[1]);
                Renderer::Unmap(m_ReductionStagingBuffer.GetHandle(), MapType::Read);
            }
        }

        float4x4 viewProjection = math::mul(view, projection);
        float4x4 invViewProj = math::inverse(viewProjection);

        float3 lightDirection(0, -1.f, 0);
        m_DirectionalLightSMQuery.each([&](const WorldTransform& transform, const DirectionalLightShadowMap& shadowMap) {
            lightDirection = transform.Value[2].xyz;

            float3 up = fabsf(math::dot(lightDirection, float3(0, 1, 0))) > 0.99f
                            ? float3(0, 0, 1)
                            : float3(0, 1, 0);
            float4x4 lightView = float4x4::look_at(float3(0, 0, 0), lightDirection, up);

            float nearZ = 0.1f;
            float farZ = 1000.f;

            CascadeShadowData shadowData{};

            // Depth Reduction Compute (Uses depth of frame N - 1)
            static float4 smoothedSplits = { nearZ * 0.25f, nearZ * 0.5f, nearZ * 0.75f, farZ };

            float minNDC = m_ReductionReadbackReady ? m_LastMinNDC : 0.0f;
            float maxNDC = m_ReductionReadbackReady ? m_LastMaxNDC : 1.0f;

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

                float p       = (c + 1.0f) / static_cast<float>(k_NumCascades);
                float1 logSplit = minView * powf(ratio, p);
                float1 linSplit = minView + (maxView - minView) * p;
                targetSplits[c] = math::lerp(linSplit, logSplit, lambda);
            }

            // Temporal smoothing, prevents split jumping
            smoothedSplits = math::lerp(smoothedSplits, targetSplits, 0.15f);

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
                float splitNear = c == 0 ? nearZ : smoothedSplits[c - 1];
                float splitFar  = smoothedSplits[c];

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
                    m_LightViewProjectionBuffer.GetHandle(),
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
                shadowData.View = view;
                shadowData.SplitDepths = smoothedSplits;

                Renderer::UpdateBuffer(
                    m_CascadeShadowDataBuffer.GetHandle(),
                    0,
                    std::as_bytes(Span(&shadowData, 1))
                );

                Renderer::BindVariableByName(
                    ShaderType::Fragment,
                    m_ShadowMaskSRB.GetHandle(),
                    "SceneDepth",
                    sceneDepthSRV
                );

                Renderer::BindVariableByName(
                    ShaderType::Fragment,
                    m_ShadowMaskSRB.GetHandle(),
                    "ShadowMaps",
                    Renderer::TextureGetDefaultView(shadowMap.ShadowMaps.GetHandle(), TextureViewType::ShaderResource)
                );

                Renderer::BindPipelineState(m_ShadowMaskPSO.GetHandle());
                Renderer::CommitShaderResources(m_ShadowMaskSRB.GetHandle(), ResourceStateTransitionMode::Transition);

                BeginRenderPassAttribs shadowMaskPassAttribs{};
                shadowMaskPassAttribs.RenderPassHandle = m_ShadowMaskRenderPass.GetHandle();
                shadowMaskPassAttribs.FrameBufferHandle = shadowMaskFB;
                ClearValue clear[1];
                clear->Color = Color::White();
                shadowMaskPassAttribs.ClearColors = clear;

                // LoadOp = Load, so no clear value needed
                Renderer::BeginRenderPass(shadowMaskPassAttribs);

                // No vertex buffer, shader generates the triangle from SV_VertexID
                DrawAttribs draw{};
                draw.NumVertices = 3;
                Renderer::Draw(draw);

                Renderer::EndRenderPass();
            }
        });

        Globals globals{};
        globals.ViewProjection = viewProjection;
        globals.LightDirection = float4(lightDirection, 0);

        Renderer::UpdateBuffer(
            m_GlobalBuffer,
            0,
            std::as_bytes(Span(&globals, 1))
        );

        Renderer::BeginRenderPass(beginRenderPassAttribs);

        uint32_t i = 0;
        while (i < m_DrawCalls.size()) {
            //
            // PIPELINE RANGE
            //

            const PipelineStateHandle& pipelineHandle = m_DrawCalls[i].PipelineState;
            const ShaderResourceBindingHandle& srb = m_DrawCalls[i].SRB;

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

            Renderer::CommitShaderResources(srb, ResourceStateTransitionMode::None);

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

        Renderer::BindVariableByName(
            ShaderType::Compute,
            m_ShadowComputeSRB.GetHandle(),
            "DepthTex",
            sceneDepthSRV
        );

        Renderer::BindPipelineState(m_ShadowComputePSO.GetHandle());
        Renderer::CommitShaderResources(m_ShadowComputeSRB.GetHandle(), ResourceStateTransitionMode::Transition);

        const TextureSpecification* sceneDepthSpec = Renderer::GetSpecification(Renderer::GetTexture(sceneDepthSRV));
        const uint32_t width = sceneDepthSpec->Width;
        const uint32_t height = sceneDepthSpec->Height;

        const auto& groupSize = m_DepthReductionCompute->GetThreadGroupSize();
        const uint32_t groupsX = (width  + groupSize[0] - 1) / groupSize[0];
        const uint32_t groupsY = (height + groupSize[1] - 1) / groupSize[1];

        DispatchComputeAttribs dispatchComputeAttribs;
        dispatchComputeAttribs.ThreadGroupCountX = groupsX;
        dispatchComputeAttribs.ThreadGroupCountY = groupsY;
        dispatchComputeAttribs.ThreadGroupCountZ = 1;

        Renderer::DispatchCompute(dispatchComputeAttribs);

        Renderer::CopyBuffer(
            m_ReductionOutBuffer.GetHandle(),
            0,
            ResourceStateTransitionMode::Transition,
            m_ReductionStagingBuffer.GetHandle(),
            0,
            sizeof(uint64_t),
            ResourceStateTransitionMode::Transition
        );

        m_ReductionReadbackReady = true;
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

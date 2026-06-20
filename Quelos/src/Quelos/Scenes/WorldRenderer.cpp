//
// Created by lasovar on 5/24/26.
//

#include "WorldRenderer.h"

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
            m_GpuBufferView = Renderer::GetDefaultBufferView(m_GPUBuffer);

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

        attachments[2].Format = ImageFormat::DEPTH32Float;
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
        m_InstancesBufferView = Renderer::GetDefaultBufferView(m_InstancesGpuBuffer);

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

        m_DrawCalls.clear();

        m_World = &world;
    }

    void WorldRenderer::Begin(
        const BeginRenderPassAttribs& beginRenderPassAttribs, const float4x4& viewProjection
    ) {
        QS_CORE_ASSERT(m_World, "WorldRenderer `m_World` is nullptr!");
        bool isDirty = false;

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

            if (!Renderer::IsAlive(pipelineStateComponent.PSO.GetHandle())) {
                m_PipelineStates.erase(pipelineStateComponent.ShaderID);
                entity.remove<PipelineStateComponent>();
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
                if (Renderer::IsAlive(it->second.PSO)) {
                    entity.emplace<PipelineStateComponent>(
                        ResourceRef(it->second.PSO),
                        ResourceRef(it->second.SRB),
                        it->second.MaterialRegistry.Add(meshRenderer.Material),
                        shader.GetAssetID()
                    );

                    return;
                }

                m_PipelineStates.erase(it);
            }

            GraphicsPipelineStateCreateInfo pipelineStateCreateInfo;
            pipelineStateCreateInfo.Name = shader.GetName();

            pipelineStateCreateInfo.GraphicsPipeline.RenderPass = m_RenderPass;
            pipelineStateCreateInfo.GraphicsPipeline.RasterizerSpec.CullMode = CullMode::Back;
            pipelineStateCreateInfo.GraphicsPipeline.RasterizerSpec.FrontCounterClockwise = true;
            pipelineStateCreateInfo.GraphicsPipeline.DepthStencilSpec.DepthEnable = true;
            pipelineStateCreateInfo.GraphicsPipeline.DepthStencilSpec.DepthFunc = ComparisonFunc::LessEqual;

            pipelineStateCreateInfo.GraphicsPipeline.SampleSpec.Count = SampleCount::x4;

            LayoutElementBuilder<4> layoutBuilder{
                LayoutElement{0, 0, ValueType::Float3},
                LayoutElement{1, 0, ValueType::Float3},
                LayoutElement{2, 0, ValueType::Float3},
                LayoutElement{3, 0, ValueType::Float2}
            };

            pipelineStateCreateInfo.GraphicsPipeline.InputLayout.LayoutElements = layoutBuilder;

            pipelineStateCreateInfo.VertexShader = shader.GetVertexShaderHandle();
            pipelineStateCreateInfo.FragmentShader = shader.GetFragmentShaderHandle();

            SmallVec<ShaderResourceVariableSpec, 4> vars = {
                {"global", ShaderType::VertexAndFragment, ShaderResourceVariableType::Static},
                {"Instances", ShaderType::VertexAndFragment, ShaderResourceVariableType::Mutable},
                {"g_Textures", ShaderType::Fragment, ShaderResourceVariableType::Dynamic},
            };

            const uint64_t materialSize = shader.GetMaterialSize();

            if (materialSize > 0) {
                vars.emplace_back("Materials", ShaderType::VertexAndFragment, ShaderResourceVariableType::Mutable);
            }

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
            pipelineStateCreateInfo.Spec.ResourceLayout.ImmutableSamplers = { &immutableSampler, 1 };

            PipelineStateHandle pipelineStateHandle = Renderer::CreatePipelineState(pipelineStateCreateInfo);

            Renderer::BindStaticVariableByName(pipelineStateHandle, ShaderType::Vertex, "global", m_GlobalBuffer);

            ShaderResourceBindingHandle srb = Renderer::CreateShaderResourceBinding(pipelineStateHandle, true);
            Renderer::BindVariableByName(ShaderType::Vertex, srb, "Instances", m_InstancesBufferView);
            Renderer::BindVariableByName(ShaderType::Fragment, srb, "Instances", m_InstancesBufferView);

            MaterialRegistry& materialRegistry = m_PipelineStates.try_emplace(
                shader.GetAssetID(),
                pipelineStateHandle,
                srb,
                MaterialRegistry(shader.GetName(), m_TextureRegistry, shader.GetMaterialSize())
            ).first->second.MaterialRegistry;

            entity.emplace<PipelineStateComponent>(
                ResourceRef(pipelineStateHandle),
                ResourceRef(srb),
                materialRegistry.Add(meshRenderer.Material),
                shader.GetAssetID()
            );

            shader.AddPipelineState(pipelineStateHandle);
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
                uint64_t sortKey = static_cast<uint64_t>(pipelineStateComponent.PSO.GetHandle().Index()) << 32
                    | static_cast<uint64_t>(meshRenderer.Mesh.GetAssetHandle().Index);

                m_DrawCalls.emplace_back(
                    sortKey,
                    entity,
                    &meshRenderer.Mesh.Get(),
                    pipelineStateComponent.PSO.GetHandle(),
                    pipelineStateComponent.SRB.GetHandle(),
                    pipelineStateComponent.MaterialIndex,
                    transform.Value
                );
            }
        );

        m_World->defer_end();

        for (auto& pipelineInfo : m_PipelineStates | std::views::values) {
            pipelineInfo.MaterialRegistry.FlushToGPU();

            if (pipelineInfo.MaterialRegistry.WasReallocated()) {
                Renderer::BindVariableByName(
                    ShaderType::Vertex,
                    pipelineInfo.SRB,
                    "Materials",
                    pipelineInfo.MaterialRegistry.GetBufferViewHandle()
                );

                Renderer::BindVariableByName(
                    ShaderType::Fragment,
                    pipelineInfo.SRB,
                    "Materials",
                    pipelineInfo.MaterialRegistry.GetBufferViewHandle()
                );
            }

            m_TextureRegistry.UpdateTexturesArray();

            Renderer::BindArrayByName(
                ShaderType::Fragment,
                pipelineInfo.SRB,
                "g_Textures",
                m_TextureRegistry.GetTextureViews()
            );

            Renderer::CommitShaderResources(pipelineInfo.SRB, ResourceStateTransitionMode::Transition);
        }

        m_TextureRegistry.SetDirty(false);

        std::ranges::sort(
            m_DrawCalls,
            [](auto& a, auto& b) {
                return a.SortKey < b.SortKey;
            }
        );

        Globals globals{};
        globals.ViewProjection = viewProjection;
        globals.LightDirection = float4(0, -1.f, 0, 0);

        if (m_DirectionalLightQuery.count() > 0) {
            flecs::entity directionLight = m_DirectionalLightQuery.first();
            globals.LightDirection = directionLight.get<WorldTransform>().Value[2];
        }

        Renderer::UpdateBuffer(
            m_GlobalBuffer,
            0,
            std::as_bytes(Span(&globals, 1))
        );

        Renderer::BeginRenderPass(beginRenderPassAttribs);
    }

    void WorldRenderer::Render() {
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
            // BUILD MATERIAL TABLE
            //

            /*
            materialUploadBuffer.clear();
            materialRemap.clear();
            */

            /*
            for (uint32_t j = pipelineBegin; j < pipelineEnd; ++j) {
                uint32_t materialId = m_DrawCalls[j].MaterialId;

                if (materialRemap.contains(materialId))
                    continue;

                uint32_t gpuIndex =
                    static_cast<uint32_t>(
                        materialUploadBuffer.size());

                materialRemap[materialId] =
                    gpuIndex;

                Material& material =
                    materialManager.Get(materialId);

                materialUploadBuffer.push_back(
                    BuildGpuMaterial(material));
            }

            UploadMaterialBuffer(materialUploadBuffer);
            */

            //
            // BUILD INSTANCE BUFFER ONCE
            //


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
    }

    void WorldRenderer::End() {
        Renderer::EndRenderPass();
    }

    WorldRenderer::~WorldRenderer() {
        for (auto& pipelineState : m_PipelineStates | std::views::values) {
            pipelineState.MaterialRegistry.Release();
        }

        Renderer::Destroy(m_RenderPass);
        Renderer::Destroy(m_WhiteTexture);
        Renderer::Destroy(m_MagentaTexture);
    }
}

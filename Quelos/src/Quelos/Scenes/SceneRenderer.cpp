//
// Created by lasovar on 5/24/26.
//

#include "SceneRenderer.h"

#include "Quelos/Renderer/Renderer.h"

namespace Quelos {
    void MaterialRegistry::FlushToGPU() {
        if (!m_Dirty) {
            return;
        }

        m_Dirty = false;
        m_Reallocated = false;

        if (const size_t needed = m_CPUMaterials.size(); needed > mGPUCapacity) {
            // Double capacity to avoid frequent reallocations
            mGPUCapacity = std::max(needed, mGPUCapacity * 2);
            if (mGPUCapacity == 0) mGPUCapacity = 16; // minimum

            GPUBufferSpec desc{};
            desc.Name = "MaterialBuffer";
            desc.Size = sizeof(Material) * mGPUCapacity;
            desc.Usage = Usage::Default;
            desc.BindFlags = Bind::ShaderResource;
            desc.Mode = GpuBufferMode::Structured;
            desc.ElementByteStride = sizeof(Material);

            // Upload all current data into the new buffer
            const BufferView data(reinterpret_cast<byte*>(m_CPUMaterials.data()), sizeof(Material) * needed);

            if (mGPUBuffer.IsValid()) {
                Renderer::Destroy(mGPUBuffer);
                mGPUBuffer = {};
            }

            mGPUBuffer = Renderer::CreateBuffer(desc, data);
            m_Reallocated = true; // caller must rebind
        }
        else {
            Renderer::UpdateBuffer(
                mGPUBuffer,
                0,
                BufferView(reinterpret_cast<byte*>(m_CPUMaterials.data()), sizeof(Material) * needed)
            );
        }
    }

    SceneRenderer::SceneRenderer(const flecs::world& world) : m_World(world) {
        RenderPassAttachmentSpec attachments[2];
        attachments[0].Format = ImageFormat::RGBA;
        attachments[0].SampleCount = 1;
        attachments[0].LoadOp = AttachmentLoadOp::Clear;
        attachments[0].StoreOp = AttachmentStoreOp::Store;
        attachments[0].InitialState = ResourceState::ShaderResource;
        attachments[0].FinalState = ResourceState::ShaderResource;

        attachments[1].Format = ImageFormat::DEPTH32F;
        attachments[1].SampleCount = 1;
        attachments[1].LoadOp = AttachmentLoadOp::Clear;
        attachments[1].StoreOp = AttachmentStoreOp::Discard;
        attachments[1].InitialState = ResourceState::DepthWrite;
        attachments[1].FinalState = ResourceState::DepthWrite;

        AttachmentReference colorRef = {0, ResourceState::RenderTarget};

        SubPassSpec subPassSpec;
        subPassSpec.RenderTargetAttachments = Span(&colorRef, 1);
        subPassSpec.DepthAttachment = {1, ResourceState::DepthWrite};

        RenderPassSpec renderPassSpec;
        renderPassSpec.Name = "Scene render pass";
        renderPassSpec.SubPasses = Span32(&subPassSpec, 1);
        renderPassSpec.Attachments = attachments;

        m_RenderPass = Renderer::CreateRenderPass(renderPassSpec);

        m_RenderingQuery = m_World.query<const WorldTransform&, const MeshRenderer&, const PipelineStateComponent&>();
        m_PSOQuery = m_World.query_builder<const MeshRenderer&>()
                            .without<PipelineStateComponent>()
                            .build();

        GPUBufferSpec spec;
        spec.Name = "global";
        spec.Size = sizeof(float4x4);
        spec.Usage = Usage::Default;
        spec.BindFlags = Bind::UniformBuffer;

        m_GlobalBuffer = Renderer::CreateBuffer(spec, {});

        constexpr uint32_t k_MaxInstances = 512;
        GPUBufferSpec instancesBufferSpec{};
        instancesBufferSpec.Name = "Instances";
        instancesBufferSpec.Size = sizeof(InstanceData) * k_MaxInstances;
        instancesBufferSpec.Usage = Usage::Dynamic;
        instancesBufferSpec.BindFlags = Bind::ShaderResource;
        instancesBufferSpec.Mode = GpuBufferMode::Structured;
        instancesBufferSpec.CpuAccessFlags = CpuAccess::Write;
        instancesBufferSpec.ElementByteStride = sizeof(InstanceData);

        m_InstanceBuffer = Renderer::CreateBuffer(instancesBufferSpec, {});
    }

    void SceneRenderer::Begin(
        const BeginRenderPassAttribs& beginRenderPassAttribs, const float4x4& viewProjection
    ) {
        m_World.defer_begin();

        m_PSOQuery.each([&](flecs::entity entity, const MeshRenderer& meshRenderer) {
            if (!meshRenderer.MeshData || !meshRenderer.ShaderData) {
                return;
            }

            auto it = m_PipelineStates.find(meshRenderer.ShaderData.GetAssetID());
            if (it != m_PipelineStates.end()) {
                if (Renderer::IsAlive(it->second.first)) {
                    entity.emplace<PipelineStateComponent>(
                        ResourceRef(it->second.first),
                        ResourceRef(it->second.second),
                        meshRenderer.ShaderData.GetAssetID()
                    );

                    return;
                }

                m_PipelineStates.erase(it);
            }

            GraphicsShader& shader = meshRenderer.ShaderData.Get();

            GraphicsPipelineStateCreateInfo pipelineStateCreateInfo;
            pipelineStateCreateInfo.Name = shader.GetName();

            pipelineStateCreateInfo.GraphicsPipeline.RenderPass = m_RenderPass;
            pipelineStateCreateInfo.GraphicsPipeline.RasterizerSpec.CullMode = CullMode::Back;
            pipelineStateCreateInfo.GraphicsPipeline.RasterizerSpec.FrontCounterClockwise = true;
            pipelineStateCreateInfo.GraphicsPipeline.DepthStencilSpec.DepthEnable = true;

            LayoutElementBuilder<4> layoutBuilder{
                LayoutElement{0, 0, ShaderDataType::Float3},
                LayoutElement{1, 0, ShaderDataType::Float3},
                LayoutElement{2, 0, ShaderDataType::Float3},
                LayoutElement{3, 0, ShaderDataType::Float2}
            };

            pipelineStateCreateInfo.GraphicsPipeline.InputLayout.LayoutElements = layoutBuilder;

            pipelineStateCreateInfo.VertexShader = shader.GetVertexShaderHandle();
            pipelineStateCreateInfo.FragmentShader = shader.GetFragmentShaderHandle();

            ShaderResourceVariableSpec vars[] = {
                {"global", ShaderType::VertexAndFragment, ShaderResourceVariableType::Static},
                {"Instances", ShaderType::VertexAndFragment, ShaderResourceVariableType::Mutable},
            };

            pipelineStateCreateInfo.Spec.ResourceLayout.Variables = vars;

            PipelineStateHandle pipelineStateHandle = Renderer::CreatePipelineState(pipelineStateCreateInfo);

            Renderer::BindStaticVariableByName(pipelineStateHandle, ShaderType::Vertex, "global", m_GlobalBuffer);

            ShaderResourceBindingHandle srb = Renderer::CreateShaderResourceBinding(pipelineStateHandle, true);
            Renderer::BindVariableByName(ShaderType::Vertex, srb, "Instances", m_InstanceBuffer);

            entity.emplace<PipelineStateComponent>(
                ResourceRef(pipelineStateHandle),
                ResourceRef(srb),
                meshRenderer.ShaderData.GetAssetID()
            );

            m_PipelineStates.emplace(meshRenderer.ShaderData.GetAssetID(), Pair{pipelineStateHandle, srb});
            shader.AddPipelineState(pipelineStateHandle);
        });

        m_World.defer_end();

        m_DrawCalls.clear();
        m_DrawCalls.reserve(m_RenderingQuery.count());

        m_World.defer_begin();

        m_RenderingQuery.each([&](
            flecs::entity entity, const WorldTransform& transform, const MeshRenderer& meshRenderer,
            const PipelineStateComponent& pipelineStateComponent
        ) {
                if (!meshRenderer.MeshData
                    || !meshRenderer.ShaderData
                    || meshRenderer.ShaderData.GetAssetID() != pipelineStateComponent.ShaderID
                    || !Renderer::IsAlive(pipelineStateComponent.PSO.GetHandle())
                ) {
                    entity.remove<PipelineStateComponent>();
                    return;
                }

                uint64_t sortKey = static_cast<uint64_t>(pipelineStateComponent.PSO.GetHandle().Index()) << 32
                    | static_cast<uint64_t>(meshRenderer.MeshData.GetAssetHandle().Index);

                m_DrawCalls.emplace_back(
                    sortKey,
                    &meshRenderer.MeshData.Get(),
                    pipelineStateComponent.PSO.GetHandle(),
                    pipelineStateComponent.SRB.GetHandle(),
                    transform.Value
                );
            });

        m_World.defer_end();

        std::ranges::sort(
            m_DrawCalls,
            [](auto& a, auto& b) {
                return a.SortKey < b.SortKey;
            }
        );

        Renderer::UpdateBuffer(
            m_GlobalBuffer,
            0,
            Span(reinterpret_cast<const byte*>(math::value_ptr(viewProjection)), sizeof(float4x4))
        );

        Renderer::BeginRenderPass(beginRenderPassAttribs);
    }

    void SceneRenderer::Render() const {
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

            Renderer::CommitShaderResources(srb);

            uint32_t j = pipelineBegin;
            uint32_t instanceOffset = 0;

            while (j < pipelineEnd) {
                Mesh* mesh = m_DrawCalls[j].Mesh;

                const uint32_t meshBegin = j;

                Vec<InstanceData> pipelineInstanceBuffer;
                while (j < pipelineEnd && m_DrawCalls[j].Mesh == mesh) {
                    const DrawCall& drawItem = m_DrawCalls[j];

                    pipelineInstanceBuffer.push_back({
                        .Transform = drawItem.Transform,
                        //.MaterialId = materialRemap[drawItem.MaterialId],
                    });

                    ++j;
                }

                void* mapped;
                Renderer::Map(m_InstanceBuffer, MapType::Write, MapFlags::Discard, mapped);
                memcpy(mapped, pipelineInstanceBuffer.data(), pipelineInstanceBuffer.size() * sizeof(InstanceData));
                Renderer::Unmap(m_InstanceBuffer, MapType::Write);

                const uint32_t instanceCount = j - meshBegin;

                Renderer::BindVertexBuffer(mesh->GetVertexBuffer(), 0);
                Renderer::BindIndexBuffer(mesh->GetIndexBuffer());

                DrawIndexedAttribs attribs;
                attribs.Flags = DrawFlags::VerifyAll;
                attribs.IndexType = ShaderDataType::UInt16;
                attribs.NumIndices = mesh->GetIndices().size();
                attribs.NumInstances = instanceCount;
                attribs.FirstInstanceLocation = 0;

                Renderer::DrawIndexed(attribs);

                instanceOffset += instanceCount;
            }
        }
    }

    void SceneRenderer::End() {
        Renderer::EndRenderPass();
    }

    SceneRenderer::~SceneRenderer() {
        Renderer::Destroy(m_RenderPass);
    }
}

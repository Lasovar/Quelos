#include "qspch.h"
#include "SceneWorkspace.h"

#include "imgui_internal.h"
#include "AssetManagement/AssetImporters/SceneImporter.h"
#include "AssetManagement/AssetImporters/ShaderImporter.h"

using namespace magic_enum::bitwise_operators;

namespace QuelosEditor {
    SceneWorkspace::SceneWorkspace(UndoSystem& undoSystem, const AssetMetadata& assetMetadata)
        : Workspace(std::string(FS::Stem(assetMetadata.FilePath)), undoSystem),
          m_Scene(SceneImporter::ImportScene(assetMetadata.Handle, assetMetadata)),
          m_SceneViewportPanel("Scene View", *this, m_Scene->GetRenderPass(), 1, 1),
          m_InspectorPanel(m_Scene, *this, undoSystem),
          m_EntityHierarchyPanel(m_Scene, *this, undoSystem)
    {
        m_WorkspaceID = ImHashStr((m_Scene->GetName() + "_Dockspace").c_str());
        m_DefaultWorkspaceDockingCondition = ImGuiCond_Appearing;
        m_ShouldDock = true;

        m_GameViewportPanel = ViewportPanel("Game View", m_Scene->GetRenderPass(), 1, 1);

        m_EditorCamera = EditorCamera(60.0f, 1.0f, 0.1f, 1000.0f);

        m_SceneSerializer = SceneSerializer(m_Scene, assetMetadata.FilePath);
        m_UndoSystem.AddSceneSerializer(m_Scene, &m_SceneSerializer);

        m_ContentBrowserPanel.Init();

        // TODO: Editor Scene view renderer

        TextureSpecification idDesc;
        idDesc.Name = "ActorIDBuffer";
        idDesc.Type = TextureType::Texture2D;
        idDesc.Width = 1;
        idDesc.Height = 1;
        idDesc.Format = ImageFormat::R32UInt;
        idDesc.BindFlags = Bind::RenderTarget;
        idDesc.Usage = Usage::Default;

        m_IDTexture = Renderer::CreateTexture(idDesc);

        TextureSpecification stagingDesc = idDesc;
        stagingDesc.Usage = Usage::Staging;
        stagingDesc.BindFlags = Bind::None;
        stagingDesc.CpuAccessFlags = CpuAccess::Read;
        m_IDStagingTexture = Renderer::CreateTexture(stagingDesc);

        TextureSpecification idDepthSpec;
        idDepthSpec.Name = "ActorIDDepth";
        idDepthSpec.Type = TextureType::Texture2D;
        idDepthSpec.Width = 1;
        idDepthSpec.Height = 1;
        idDepthSpec.Format = ImageFormat::DEPTH32Float;
        idDepthSpec.BindFlags = Bind::DepthStencil;
        idDepthSpec.Usage = Usage::Default;
        m_IDDepthTexture = Renderer::CreateTexture(idDepthSpec);

        RenderPassAttachmentSpec idAttachments[2];
        idAttachments[0].Format = ImageFormat::R32UInt;
        idAttachments[0].SampleCount = 1;
        idAttachments[0].LoadOp = AttachmentLoadOp::Clear;
        idAttachments[0].StoreOp = AttachmentStoreOp::Store;
        idAttachments[0].InitialState = ResourceState::RenderTarget;
        idAttachments[0].FinalState = ResourceState::CopySource;

        idAttachments[1].Format = ImageFormat::DEPTH32Float;
        idAttachments[1].SampleCount = 1;
        idAttachments[1].LoadOp = AttachmentLoadOp::Clear;
        idAttachments[1].StoreOp = AttachmentStoreOp::Discard;
        idAttachments[1].InitialState = ResourceState::DepthWrite;
        idAttachments[1].FinalState = ResourceState::DepthWrite;

        AttachmentReference colorRef{0, ResourceState::RenderTarget};
        AttachmentReference depthRef{1, ResourceState::DepthWrite};

        SubPassSpec subpass{};
        subpass.RenderTargetAttachments = {&colorRef, 1};
        subpass.pDepthAttachment = &depthRef;

        RenderPassSpec desc{};
        desc.Name = "ActorIDPass";
        desc.Attachments = idAttachments;
        desc.SubPasses = {&subpass, 1};
        m_ActorIDRenderPass = Renderer::CreateRenderPass(desc);

        const TextureViewHandle idFbAttachments[] = {
            Renderer::GetTextureView(m_IDTexture.GetHandle(), TextureViewType::RenderTarget),
            Renderer::GetTextureView(m_IDDepthTexture.GetHandle(), TextureViewType::DepthStencil),
        };

        FrameBufferSpec fbDesc;
        fbDesc.Name = "ActorIDFrameBuffer";
        fbDesc.RenderPassHandle = m_ActorIDRenderPass.GetHandle();
        fbDesc.Attachments = idFbAttachments;
        fbDesc.Size = { 1, 1 };

        m_IDFrameBuffer = Renderer::CreateFrameBuffer(fbDesc);

        AssetMetadata metadata;
        metadata.FilePath = "Assets/shaders/EntitySelection.slang";
        metadata.Type = GraphicsShader::GetStaticType();

        ShaderImporter::Cook(metadata);
        ShaderImporter::Import(GetIDShader(), metadata);

        GraphicsShader* shader = GetIDShader();
        GraphicsPipelineStateCreateInfo pipelineStateCreateInfo;
        pipelineStateCreateInfo.Name = shader->GetName();

        pipelineStateCreateInfo.GraphicsPipeline.RenderPass = m_ActorIDRenderPass.GetHandle();
        pipelineStateCreateInfo.GraphicsPipeline.RasterizerSpec.CullMode = CullMode::Back;
        pipelineStateCreateInfo.GraphicsPipeline.RasterizerSpec.FrontCounterClockwise = true;
        pipelineStateCreateInfo.GraphicsPipeline.DepthStencilSpec.DepthEnable = true;
        pipelineStateCreateInfo.GraphicsPipeline.DepthStencilSpec.DepthWriteEnable = true;

        pipelineStateCreateInfo.GraphicsPipeline.SampleSpec.Count = SampleCount::x1;

        LayoutElementBuilder<4> layoutBuilder{
            LayoutElement{0, 0, ValueType::Float3},
            LayoutElement{1, 0, ValueType::Float3},
            LayoutElement{2, 0, ValueType::Float3},
            LayoutElement{3, 0, ValueType::Float2}
        };

        pipelineStateCreateInfo.GraphicsPipeline.InputLayout.LayoutElements = layoutBuilder;

        pipelineStateCreateInfo.VertexShader = shader->GetVertexShaderHandle();
        pipelineStateCreateInfo.FragmentShader = shader->GetFragmentShaderHandle();

        SmallVec<ShaderResourceVariableSpec, 4> vars = {
            {"global", ShaderType::VertexAndFragment, ShaderResourceVariableType::Static},
            {"Instances", ShaderType::VertexAndFragment, ShaderResourceVariableType::Mutable},
        };

        pipelineStateCreateInfo.Spec.ResourceLayout.Variables = vars;

        m_IDPSO = Renderer::CreatePipelineState(pipelineStateCreateInfo);
        SceneRenderer& sceneRenderer = m_Scene->GetSceneRenderer();
        Renderer::BindStaticVariableByName(
            m_IDPSO.GetHandle(),
            ShaderType::Vertex,
            "global",
            sceneRenderer.GetGlobalBuffer()
        );

        m_IDSRB = Renderer::CreateShaderResourceBinding(m_IDPSO.GetHandle(), true);
        Renderer::BindVariableByName(
            ShaderType::Vertex,
            m_IDSRB.GetHandle(),
            "Instances",
            sceneRenderer.GetInstancesGpuBuffer()
        );

        shader->AddPipelineState(m_IDPSO.GetHandle());
    }

    void SceneWorkspace::SetSelectEntity(const Entity entity) {
        m_SelectedEntity = entity;
        for (auto& callback : m_OnSelectionChangedCallbacks) {
            callback(entity);
        }
    }

    void SceneWorkspace::Tick(const float deltaTime) {
        if (!m_SceneViewportPanel.IsViewportFocused() && !m_SceneViewportPanel.IsViewportHovered()) {
            m_EditorCamera.ClearInput();
        }

        m_EditorCamera.OnUpdate(deltaTime);

        m_Scene->Tick(deltaTime);

        if (m_SceneViewportPanel.ShouldDraw()) {
            m_EditorCamera.SetViewportFocused(m_SceneViewportPanel.IsViewportFocused());
            m_EditorCamera.SetViewportHovered(m_SceneViewportPanel.IsViewportHovered());

            if (m_SceneViewportPanel.ResizeIfNeeded()) {
                const float2 size = m_SceneViewportPanel.GetViewportSize();
                m_EditorCamera.SetViewportSize(size.x, size.y);
                Renderer::FrameBufferResize(m_IDFrameBuffer.GetHandle(), size.x, size.y);
                Renderer::TextureResize(m_IDStagingTexture.GetHandle(), size.x, size.y);
            }

            ClearValue clearValues[3];
            clearValues[0].Format = ImageFormat::RGBA8UNorm;
            clearValues[0].Color = {0.2667f, 0.2000f, 0.3333f, 1.0000f};

            clearValues[1] = {};

            clearValues[2].Format = ImageFormat::DEPTH32Float;
            clearValues[2].DepthStencil.Depth = 1.0f;

            BeginRenderPassAttribs attribs;
            attribs.ClearColors = clearValues;

            attribs.FrameBufferHandle = m_SceneViewportPanel.GetFrameBuffer()->GetHandle();
            attribs.RenderPassHandle = m_Scene->GetRenderPass();

            m_Scene->StartRender(m_EditorCamera.GetViewMatrix(), m_EditorCamera.GetProjection(), attribs);

            m_Scene->Render();

            m_Scene->EndRender();

            if (m_SceneViewportPanel.IsSelectRequest()) {
                ClearValue idClearValues[2];
                idClearValues[0].Format = ImageFormat::R32UInt;

                uint32_t invalidId = std::numeric_limits<uint32_t>::max();
                float clearAsFloat;
                memcpy(&clearAsFloat, &invalidId, 4);

                idClearValues[0].Color = {clearAsFloat};

                idClearValues[1].Format = ImageFormat::DEPTH32Float;
                idClearValues[1].DepthStencil.Depth = 1.0f;

                attribs.FrameBufferHandle = m_IDFrameBuffer.GetHandle();
                attribs.RenderPassHandle = m_ActorIDRenderPass.GetHandle();
                attribs.ClearColors = idClearValues;

                Renderer::BeginRenderPass(attribs);

                const Vec<DrawCommand>& drawCalls = m_Scene->GetSceneRenderer().GetDrawCalls();
                const GpuBufferHandle& instancesGpuBuffer = m_Scene->GetSceneRenderer().GetInstancesGpuBuffer();

                Renderer::BindPipelineState(m_IDPSO.GetHandle());
                Renderer::CommitShaderResources(m_IDSRB.GetHandle(), ResourceStateTransitionMode::None);

                uint32_t i = 0;
                while (i < drawCalls.size()) {
                    //
                    // PIPELINE RANGE
                    //

                    uint32_t pipelineBegin = i;
                    const PipelineStateHandle& pipelineHandle = drawCalls[i].PipelineState;

                    while (i < drawCalls.size() && drawCalls[i].PipelineState == pipelineHandle) {
                        ++i;
                    }

                    const uint32_t pipelineEnd = i;

                    //
                    // BIND PIPELINE
                    //


                    //
                    // DRAW MESH RANGES
                    //


                    uint32_t j = pipelineBegin;
                    uint32_t instanceOffset = 0;

                    while (j < pipelineEnd) {
                        const Mesh* mesh = drawCalls[j].Mesh;

                        void* mapped;
                        Renderer::Map(instancesGpuBuffer, MapType::Write, MapFlags::Discard, mapped);
                        auto* instances = static_cast<InstanceData*>(mapped);

                        uint32_t instanceCount = 0;
                        while (j < pipelineEnd && instanceCount < k_MaxInstances && drawCalls[j].Mesh == mesh) {
                            instances[instanceCount++] = InstanceData{
                                .Transform = drawCalls[j].Transform,
                                .MaterialId = j + 1,
                                .EntityIndex = j + 1
                            };

                            ++j;
                        }

                        Renderer::Unmap(instancesGpuBuffer, MapType::Write);

                        Renderer::BindVertexBuffer(mesh->GetVertexBuffer(), 0);
                        Renderer::BindIndexBuffer(mesh->GetIndexBuffer());

                        DrawIndexedAttribs indexedAttribs;
                        indexedAttribs.Flags = DrawFlags::VerifyAll;
                        indexedAttribs.IndexType = ValueType::UInt16;
                        indexedAttribs.NumIndices = mesh->GetIndices().size();
                        indexedAttribs.NumInstances = instanceCount;
                        indexedAttribs.FirstInstanceLocation = 0;

                        Renderer::DrawIndexed(indexedAttribs);

                        instanceOffset += instanceCount;
                    }
                }

                Renderer::EndRenderPass();

                CopyTextureAttribs copy;
                copy.Source = m_IDTexture.GetHandle();
                copy.Destination = m_IDStagingTexture.GetHandle();
                copy.SourceTransitionMode = ResourceStateTransitionMode::Transition;
                copy.DestinationTransitionMode = ResourceStateTransitionMode::Transition;
                Renderer::CopyTexture(copy);

                MappedTextureSubresource mapped;
                Renderer::MapTextureSubresource(
                    m_IDStagingTexture.GetHandle(),
                    0,
                    0,
                    Map::Read,
                    MapFlags::DoNotWait,
                    mapped
                );

                if (mapped.Data) {
                    const uint2& position = m_SceneViewportPanel.GetSelectRequestPosition();
                    auto pixels = static_cast<uint32_t*>(mapped.Data);

                    uint32_t rowPitch = mapped.Stride / sizeof(uint32_t); // Stride is in bytes
                    uint32_t pickedId = pixels[position.y * rowPitch + position.x];

                    if (pickedId != 0 && pickedId <= drawCalls.size()) {
                        SetSelectEntity(drawCalls[pickedId - 1].Entity);
                    } else {
                        SetSelectEntity({});
                    }

                    Renderer::UnmapTextureSubresource(m_IDStagingTexture.GetHandle(), 0, 0);
                }

                m_SceneViewportPanel.SetSelectRequest(false);
            }
        }

        if (m_GameViewportPanel.ShouldDraw()) {
            if (m_GameViewportPanel.ResizeIfNeeded()) {
                m_Scene->OnViewportResized(m_GameViewportPanel.GetViewportSize());
            }

            ClearValue clearValues[3];
            clearValues[0].Format = ImageFormat::RGBA8UNorm;
            clearValues[0].Color = {0.2667f, 0.2000f, 0.3333f, 1.0000f};

            clearValues[1] = {};

            clearValues[2].Format = ImageFormat::DEPTH32Float;
            clearValues[2].DepthStencil.Depth = 1.0f;

            BeginRenderPassAttribs attribs;
            attribs.FrameBufferHandle = m_GameViewportPanel.GetFrameBuffer()->GetHandle();
            attribs.RenderPassHandle = m_Scene->GetRenderPass();
            attribs.ClearColors = clearValues;

            m_Scene->StartRender(attribs);
            m_Scene->Render();
            m_Scene->EndRender();
        }
    }

    /*void DecomposeMatrix(const float4x4& m, float3& position, quaternion& rotation, float3& scale) {
        // Position is just the last column
        position = float3(m[3][0], m[3][1], m[3][2]);

        // Scale is the length of each basis vector (column)
        float3 col0 = float3(m[0][0], m[0][1], m[0][2]);
        float3 col1 = float3(m[1][0], m[1][1], m[1][2]);
        float3 col2 = float3(m[2][0], m[2][1], m[2][2]);

        scale.x = math::length(col0);
        scale.y = math::length(col1);
        scale.z = math::length(col2);

        // Remove scale from rotation matrix
        float3x3 rotMat;
        rotMat[0] = col0 / scale.x;
        rotMat[1] = col1 / scale.y;
        rotMat[2] = col2 / scale.z;

        // Build quaternion from rotation matrix
        rotation = quaternion(rotMat);
    }*/

    void SceneWorkspace::WorkspaceContents() {
        m_SceneViewportPanel.SetFrame(m_SelectedEntity, m_EditorCamera.GetViewMatrix(), m_EditorCamera.GetProjection());
        m_SceneViewportPanel.OnImGuiRender(m_WorkspaceID, m_WorkspaceClass);

        m_GameViewportPanel.OnImGuiRender(m_WorkspaceID, m_WorkspaceClass);
        m_EntityHierarchyPanel.OnImGuiRender(m_WorkspaceID, m_WorkspaceClass);
        m_InspectorPanel.OnImGuiRender(m_WorkspaceID, m_WorkspaceClass);
        m_ContentBrowserPanel.OnImGuiRender(m_WorkspaceID, m_WorkspaceClass);
    }

    void SceneWorkspace::OnEvent(Event& event) {
        m_EditorCamera.OnEvent(event);

        EventDispatcher dispatcher(event);
        dispatcher.Dispatch<KeyPressedEvent>([this](const KeyPressedEvent& e) {
            switch (e.GetKeyCode()) {
            case KeyCode::LeftControl:
            case KeyCode::RightControl:
                m_CtrlKey = true;
                break;
            case KeyCode::LeftShift:
            case KeyCode::RightShift:
                m_ShiftKey = true;
                break;
            case KeyCode::S:
                if (!e.IsRepeat()) {
                    if (m_CtrlKey) {
                        if (m_ShiftKey) {
                            m_SceneSerializer.BakePatches();
                        }
                        else {
                            m_SceneSerializer.SerializePatches();
                        }
                    }
                }
            default:
                break;
            }

            return false;
        });


        dispatcher.Dispatch<KeyReleasedEvent>([this](const KeyReleasedEvent& e) {
            switch (e.GetKeyCode()) {
            case KeyCode::LeftControl:
            case KeyCode::RightControl:
                m_CtrlKey = false;
                break;
            case KeyCode::LeftShift:
            case KeyCode::RightShift:
                m_ShiftKey = false;
                break;
            default:
                break;
            }

            return false;
        });
    }
}

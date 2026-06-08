#include "qspch.h"
#include "SceneWorkspace.h"

#include "EditorLayer.h"
#include "imgui_internal.h"
#include "AssetManagement/AssetImporters/SceneImporter.h"
#include "AssetManagement/AssetImporters/ShaderImporter.h"
#include "Quelos/Scenes/SceneSnapshot.h"

using namespace magic_enum::bitwise_operators;

namespace QuelosEditor {
    SceneWorkspace::SceneWorkspace(UndoSystem& undoSystem, const AssetMetadata& assetMetadata)
        : Workspace(std::string(FS::Stem(assetMetadata.FilePath)), undoSystem),
          m_EditorScene(SceneImporter::ImportScene(assetMetadata.Handle, assetMetadata, m_World)),
          m_ActiveScene(m_EditorScene),
          m_WorldRenderer(m_World),
          m_GameViewportPanel("Game View", *this, m_WorldRenderer.GetRenderPass(), 1, 1),
          m_SceneViewportPanel("Scene View", *this, m_WorldRenderer.GetRenderPass(), 1, 1),
          m_InspectorPanel(m_ActiveScene, *this, undoSystem),
          m_EntityHierarchyPanel(m_ActiveScene, *this, undoSystem)
    {
        m_WorkspaceID = ImHashStr((m_ActiveScene->GetName() + "_Dockspace").c_str());
        m_DefaultWorkspaceDockingCondition = ImGuiCond_Appearing;
        m_ShouldDock = true;

        m_EditorCamera = EditorCamera(60.0f, 1.0f, 0.1f, 1000.0f);

        m_SceneSerializer = SceneSerializer(m_EditorScene, assetMetadata.FilePath);
        m_UndoSystem.AddSceneSerializer(m_EditorScene, &m_SceneSerializer);

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

        shader->AddPipelineState(m_IDPSO.GetHandle());

        Renderer::BindStaticVariableByName(
            m_IDPSO.GetHandle(),
            ShaderType::Vertex,
            "global",
            m_WorldRenderer.GetGlobalBuffer()
        );

        m_IDSRB = Renderer::CreateShaderResourceBinding(m_IDPSO.GetHandle(), true);
        Renderer::BindVariableByName(
            ShaderType::Vertex,
            m_IDSRB.GetHandle(),
            "Instances",
            m_WorldRenderer.GetInstancesGpuBuffer()
        );
    }

    void SceneWorkspace::SetSelectEntity(const Entity entity) {
        m_SelectedEntity = entity;
        for (auto& callback : m_OnSelectionChangedCallbacks) {
            callback(entity);
        }
    }

    void SceneWorkspace::Tick(const float deltaTime) {
        if (m_PlayRequest.Resolve()) {
            OnScenePlay();
        }

        if (m_StopRequest.Resolve()) {
            OnSceneStop();
        }

        if (!m_SceneViewportPanel.IsViewportFocused() && !m_SceneViewportPanel.IsViewportHovered()) {
            m_EditorCamera.ClearInput();
        }

        m_EditorCamera.OnUpdate(deltaTime);

        m_ActiveScene->Tick(deltaTime);

        if (m_SceneViewportPanel.ShouldDraw()) {
            if (m_PickRequest.Resolve()) {
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

                    if (pickedId < m_PickIds.size()) {
                        SetSelectEntity(m_PickIds[pickedId]);
                    } else {
                        SetSelectEntity({});
                    }

                    Renderer::UnmapTextureSubresource(m_IDStagingTexture.GetHandle(), 0, 0);
                }
            }

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
            attribs.RenderPassHandle = m_WorldRenderer.GetRenderPass();

            m_WorldRenderer.Begin(attribs, m_EditorCamera.GetViewProjection());
            m_WorldRenderer.Render();
            m_WorldRenderer.End();

            if (m_SceneViewportPanel.SelectRequest().Resolve()) {
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

                const Vec<DrawCommand>& drawCalls = m_WorldRenderer.GetDrawCalls();
                const GpuBufferHandle& instancesGpuBuffer = m_WorldRenderer.GetInstancesGpuBuffer();

                Renderer::BindPipelineState(m_IDPSO.GetHandle());
                Renderer::CommitShaderResources(m_IDSRB.GetHandle(), ResourceStateTransitionMode::None);

                m_PickIds.clear();

                uint32_t i = 0;
                while (i < drawCalls.size()) {
                    // PIPELINE RANGE

                    uint32_t pipelineBegin = i;
                    const PipelineStateHandle& pipelineHandle = drawCalls[i].PipelineState;

                    while (i < drawCalls.size() && drawCalls[i].PipelineState == pipelineHandle) {
                        ++i;
                    }

                    const uint32_t pipelineEnd = i;

                    // DRAW MESH RANGES

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
                                .MaterialId = j,
                                .EntityIndex = j
                            };

                            QS_ASSERT(m_PickIds.size() == j);
                            m_PickIds.push_back(drawCalls[j].Entity);

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

                m_PickRequest = true;
            }
        }

        if (m_GameViewportPanel.ShouldDraw()) {
            if (m_GameViewportPanel.ResizeIfNeeded()) {
                m_ActiveScene->OnViewportResized(m_GameViewportPanel.GetViewportSize());
            }

            ClearValue clearValues[3];
            clearValues[0].Format = ImageFormat::RGBA8UNorm;
            clearValues[0].Color = {0.2667f, 0.2000f, 0.3333f, 1.0000f};

            clearValues[1] = {};

            clearValues[2].Format = ImageFormat::DEPTH32Float;
            clearValues[2].DepthStencil.Depth = 1.0f;

            BeginRenderPassAttribs attribs;
            attribs.FrameBufferHandle = m_GameViewportPanel.GetFrameBuffer()->GetHandle();
            attribs.RenderPassHandle = m_WorldRenderer.GetRenderPass();
            attribs.ClearColors = clearValues;

            m_WorldRenderer.Begin(attribs, m_ActiveScene->GetViewProjection());
            m_WorldRenderer.Render();
            m_WorldRenderer.End();
        }
    }

    void SceneWorkspace::WorkspaceContents() {
        m_GameViewportPanel.OnImGuiRender(m_WorkspaceID, m_WorkspaceClass);

        m_SceneViewportPanel.SetFrame(m_SelectedEntity, m_EditorCamera.GetViewMatrix(), m_EditorCamera.GetProjection());
        m_SceneViewportPanel.OnImGuiRender(m_WorkspaceID, m_WorkspaceClass);

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

    void SceneWorkspace::ScenePlay() {
        m_PlayRequest = true;
    }

    void SceneWorkspace::SceneStop() {
        m_StopRequest = true;
    }

    void SceneWorkspace::Init() {
        m_SelectedEntity = {};

        m_PickIds.clear();

        m_EntityHierarchyPanel.SetScene(m_ActiveScene);
        m_InspectorPanel.SetScene(m_ActiveScene);

        m_ActiveScene->OnViewportResized(m_GameViewportPanel.GetViewportSize());
    }

    void SceneWorkspace::OnScenePlay() {
        m_SceneSnapshot = SceneSnapshot::Create(m_EditorScene);
        m_ActiveScene = CreateRef<Scene>(m_World);
        m_SceneSnapshot.Load(m_ActiveScene);

        m_EditorScene->Destroy();

        Init();

        m_SceneState = SceneState::Play;
        EditorLayer::Get().AddPlayingScene(this, m_ActiveScene->GetName());
    }

    void SceneWorkspace::OnSceneStop() {
        m_EditorScene->Init();
        m_SceneSnapshot.Load(m_EditorScene);

        m_ActiveScene->Destroy();
        m_ActiveScene = m_EditorScene;

        Init();

        m_SceneState = SceneState::Edit;
        EditorLayer::Get().RemovePlayingScene(this);
    }
}

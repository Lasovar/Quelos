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
          m_GameViewportPanel("Game View", *this, m_WorldRenderer.GetRenderPass(), 1, 1),
          m_SceneViewportPanel("Scene View", *this, m_WorldRenderer.GetRenderPass(), 1, 1),
          m_InspectorPanel(*this, undoSystem),
          m_EntityHierarchyPanel(*this, undoSystem)
    {
        ComponentRegistry::RegisterBuiltinTypes(m_EditorWorld);
        ComponentRegistry::RegisterBuiltinTypes(m_RuntimeWorld);

        m_EditorScene = SceneImporter::ImportScene(assetMetadata.Handle, assetMetadata, m_EditorWorld);
        m_ActiveScene = m_EditorScene;

        m_InspectorPanel.SetScene(m_ActiveScene);
        m_EntityHierarchyPanel.SetScene(m_ActiveScene);

        m_WorldRenderer.SetWorld(m_EditorWorld);

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
        idDesc.BindFlags = Bind::RenderTarget | Bind::ShaderResource;
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

        LayoutElementBuilder<5> layoutBuilder{
            LayoutElement{0, 0, ValueType::Float3},
            LayoutElement{1, 0, ValueType::Float3},
            LayoutElement{2, 0, ValueType::Float3},
            LayoutElement{3, 0, ValueType::Float3},
            LayoutElement{4, 0, ValueType::Float2}
        };

        pipelineStateCreateInfo.GraphicsPipeline.InputLayout.LayoutElements = layoutBuilder;

        const GraphicsShaderPass* pass = shader->GetShaderPass("EditorEntityID");
        pipelineStateCreateInfo.VertexShader = pass->Pipelines.front().VertexShader;
        pipelineStateCreateInfo.FragmentShader = pass->Pipelines.front().FragmentShader;

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
            m_WorldRenderer.GetInstancesBufferView()
        );

        CreateOutlineMaskResources();
        CreateOutlineCompositeResources();

        m_RuntimeWorld.system<LocalTransform&>().each([](const flecs::iter& it, size_t, LocalTransform& transform) {
            transform.Rotation = math::mul(transform.Rotation, quaternion::rotation_y(it.delta_time()));
        });
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

                Renderer::TextureResize(m_IDTexture.GetHandle(), size.x, size.y);
                Renderer::TextureResize(m_IDDepthTexture.GetHandle(), size.x, size.y);
                Renderer::TextureResize(m_IDStagingTexture.GetHandle(), size.x, size.y);
                Renderer::FrameBufferResize(m_IDFrameBuffer.GetHandle(), size.x, size.y);

                Renderer::TextureResize(    m_VisibleMaskMSAATexture.GetHandle(), size.x, size.y);
                Renderer::TextureResize(    m_VisibleMaskResolvedTexture.GetHandle(), size.x, size.y);
                Renderer::FrameBufferResize(m_VisibleMaskFrameBuffer.GetHandle(), size.x, size.y);

                Renderer::TextureResize(    m_FullMaskMSAATexture.GetHandle(), size.x, size.y);
                Renderer::TextureResize(    m_FullMaskResolvedTexture.GetHandle(), size.x, size.y);
                Renderer::FrameBufferResize(m_FullMaskFrameBuffer.GetHandle(), size.x, size.y);

                Renderer::FrameBufferResize(m_CompositeFrameBuffer.GetHandle(), size.x, size.y);
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

            if (m_SelectedEntity.IsValid()) {
                RunMaskPass();
                CompositePass();
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
        m_WorldRenderer.SetWorld(m_ActiveScene->GetWorld());
    }

    void SceneWorkspace::OnScenePlay() {
        m_SceneSnapshot = SceneSnapshot::Create(m_EditorScene);
        m_ActiveScene = CreateRef<Scene>(m_RuntimeWorld);
        m_SceneSnapshot.Load(m_ActiveScene);

        Init();

        m_SceneState = SceneState::Play;
        EditorLayer::Get().AddPlayingScene(this, m_ActiveScene->GetName());
    }

    void SceneWorkspace::OnSceneStop() {
        m_ActiveScene->Destroy();
        m_ActiveScene = m_EditorScene;

        Init();

        m_SceneState = SceneState::Edit;
        EditorLayer::Get().RemovePlayingScene(this);
    }

    void SceneWorkspace::CreateOutlineMaskResources() {
        AssetMetadata selectedMaskMetadata;
        selectedMaskMetadata.FilePath = "Assets/shaders/SelectedOutlineMask.slang";
        selectedMaskMetadata.Handle = AssetID("deaddead-beaf-beef-dead-deadbeafbeef");
        selectedMaskMetadata.Type = GraphicsShader::GetStaticType();

        GraphicsShader* shader = GetMaskShader();

        ShaderImporter::Cook(selectedMaskMetadata);
        ShaderImporter::Import(shader, selectedMaskMetadata);

        const GraphicsShaderPass* pass = shader->GetShaderPass("SelectedOutlineMask");

        // FULL MASK
        {
            TextureSpecification fullMaskMSAASpec;
            fullMaskMSAASpec.Format = ImageFormat::R8UNorm;
            fullMaskMSAASpec.SampleCount = SampleCount::x4;
            fullMaskMSAASpec.BindFlags = Bind::RenderTarget;
            m_FullMaskMSAATexture = Renderer::CreateTexture(fullMaskMSAASpec);

            TextureSpecification fullMaskResolvedSpec;
            fullMaskResolvedSpec.Format = ImageFormat::R8UNorm;
            fullMaskResolvedSpec.SampleCount = SampleCount::x1;
            fullMaskResolvedSpec.BindFlags = Bind::RenderTarget | Bind::ShaderResource;
            m_FullMaskResolvedTexture = Renderer::CreateTexture(fullMaskResolvedSpec);

            RenderPassAttachmentSpec fullMaskAttachments[2];

            fullMaskAttachments[0].Format = ImageFormat::R8UNorm;
            fullMaskAttachments[0].SampleCount = 4;
            fullMaskAttachments[0].LoadOp = AttachmentLoadOp::Clear;
            fullMaskAttachments[0].StoreOp = AttachmentStoreOp::Store;
            fullMaskAttachments[0].InitialState = ResourceState::RenderTarget;
            fullMaskAttachments[0].FinalState = ResourceState::RenderTarget;

            fullMaskAttachments[1].Format = ImageFormat::R8UNorm;
            fullMaskAttachments[1].SampleCount = 1;
            fullMaskAttachments[1].LoadOp = AttachmentLoadOp::Clear;
            fullMaskAttachments[1].StoreOp = AttachmentStoreOp::Store;
            fullMaskAttachments[1].InitialState = ResourceState::ResolveDest;
            fullMaskAttachments[1].FinalState = ResourceState::ShaderResource;

            AttachmentReference fullMaskColorRef { 0, ResourceState::RenderTarget };
            AttachmentReference fullMaskResolveRef{ 1, ResourceState::ResolveDest };

            SubPassSpec fullMaskSubpass;
            fullMaskSubpass.RenderTargetAttachments = Span32(&fullMaskColorRef, 1);
            fullMaskSubpass.pResolveAttachments = &fullMaskResolveRef;

            RenderPassSpec fullMaskPassSpec;
            fullMaskPassSpec.Name = "EditorOutlineFullMaskPass";
            fullMaskPassSpec.Attachments = fullMaskAttachments;
            fullMaskPassSpec.SubPasses = Span32(&fullMaskSubpass, 1);
            m_FullMaskRenderPass = Renderer::CreateRenderPass(fullMaskPassSpec);

            const TextureViewHandle fullMaskFbAttachments[] = {
                Renderer::GetTextureView(m_FullMaskMSAATexture.GetHandle(), TextureViewType::RenderTarget),
                Renderer::GetTextureView(m_FullMaskResolvedTexture.GetHandle(), TextureViewType::RenderTarget)
            };

            FrameBufferSpec fullMaskFbSpec;
            fullMaskFbSpec.RenderPassHandle = m_FullMaskRenderPass.GetHandle();
            fullMaskFbSpec.Attachments = fullMaskFbAttachments;
            fullMaskFbSpec.Size = {1, 1};
            m_FullMaskFrameBuffer = Renderer::CreateFrameBuffer(fullMaskFbSpec);

            GraphicsPipelineStateCreateInfo fullMaskPsoCI;
            fullMaskPsoCI.Name = "EditorOutlineFullMask";
            fullMaskPsoCI.GraphicsPipeline.RenderPass = m_FullMaskRenderPass.GetHandle();

            fullMaskPsoCI.VertexShader = pass->Pipelines.front().VertexShader;
            fullMaskPsoCI.FragmentShader = pass->Pipelines.front().FragmentShader;

            // No input layout, vertex shader generates positions
            LayoutElementBuilder<5> layoutBuilder{
                LayoutElement{0, 0, ValueType::Float3},
                LayoutElement{1, 0, ValueType::Float3},
                LayoutElement{2, 0, ValueType::Float3},
                LayoutElement{3, 0, ValueType::Float3},
                LayoutElement{4, 0, ValueType::Float2}
            };

            fullMaskPsoCI.GraphicsPipeline.InputLayout.LayoutElements = layoutBuilder;

            fullMaskPsoCI.GraphicsPipeline.RasterizerSpec.CullMode = CullMode::Back;
            fullMaskPsoCI.GraphicsPipeline.RasterizerSpec.FrontCounterClockwise = true;

            // No depth
            fullMaskPsoCI.GraphicsPipeline.DepthStencilSpec.DepthEnable = false;

            fullMaskPsoCI.GraphicsPipeline.SampleSpec.Count = SampleCount::x4;

            SmallVec<ShaderResourceVariableSpec, 2> vars = {
                {"global", ShaderType::VertexAndFragment, ShaderResourceVariableType::Static},
                {"Instances", ShaderType::VertexAndFragment, ShaderResourceVariableType::Mutable},
            };

            fullMaskPsoCI.Spec.ResourceLayout.Variables = vars;

            m_FullMaskPSO = Renderer::CreatePipelineState(fullMaskPsoCI);

            Renderer::BindStaticVariableByName(
                m_FullMaskPSO.GetHandle(),
                ShaderType::Vertex,
                "global",
                m_WorldRenderer.GetGlobalBuffer()
            );

            m_FullMaskSRB = Renderer::CreateShaderResourceBinding(m_FullMaskPSO.GetHandle(), true);

            Renderer::BindVariableByName(
                ShaderType::Vertex,
                m_FullMaskSRB.GetHandle(),
                "Instances",
                m_WorldRenderer.GetInstancesBufferView()
            );
        }

        // VISIBLE MASK
        {
            // MSAA mask
            TextureSpecification visibleMaskSpec;
            visibleMaskSpec.Format = ImageFormat::R8UNorm;
            visibleMaskSpec.SampleCount = SampleCount::x4;
            visibleMaskSpec.BindFlags = Bind::RenderTarget;
            m_VisibleMaskMSAATexture = Renderer::CreateTexture(visibleMaskSpec);

            // Resolved 1x mask, used by composite shader
            TextureSpecification visibleMaskResolvedSpec;
            visibleMaskResolvedSpec.Format = ImageFormat::R8UNorm;
            visibleMaskResolvedSpec.SampleCount = SampleCount::x1;
            visibleMaskResolvedSpec.BindFlags = Bind::RenderTarget | Bind::ShaderResource;
            m_VisibleMaskResolvedTexture = Renderer::CreateTexture(visibleMaskResolvedSpec);

            RenderPassAttachmentSpec visibleMaskAttachments[3];

            visibleMaskAttachments[0].Format = ImageFormat::R8UNorm;
            visibleMaskAttachments[0].SampleCount = 4;
            visibleMaskAttachments[0].LoadOp = AttachmentLoadOp::Clear;
            visibleMaskAttachments[0].StoreOp = AttachmentStoreOp::Store;
            visibleMaskAttachments[0].InitialState = ResourceState::RenderTarget;
            visibleMaskAttachments[0].FinalState = ResourceState::RenderTarget;

            visibleMaskAttachments[1].Format = ImageFormat::R8UNorm;
            visibleMaskAttachments[1].SampleCount = 1;
            visibleMaskAttachments[1].LoadOp = AttachmentLoadOp::Clear;
            visibleMaskAttachments[1].StoreOp = AttachmentStoreOp::Store;
            visibleMaskAttachments[1].InitialState = ResourceState::ResolveDest;
            visibleMaskAttachments[1].FinalState = ResourceState::ShaderResource;

            // SHARED scene depth, read-only, load existing values, don't clear/write
            visibleMaskAttachments[2].Format = ImageFormat::DEPTH32Float;
            visibleMaskAttachments[2].SampleCount = 4;
            visibleMaskAttachments[2].LoadOp = AttachmentLoadOp::Load;
            visibleMaskAttachments[2].StoreOp = AttachmentStoreOp::Discard;
            visibleMaskAttachments[2].InitialState = ResourceState::DepthRead;
            visibleMaskAttachments[2].FinalState = ResourceState::DepthRead;

            AttachmentReference visibleMaskColorRef { 0, ResourceState::RenderTarget };
            AttachmentReference visibleMaskResolveRef{ 1, ResourceState::ResolveDest };
            AttachmentReference visibleMaskDepthRef{ 2, ResourceState::DepthRead };

            SubPassSpec visibleMaskSubpass{};
            visibleMaskSubpass.RenderTargetAttachments = Span32(&visibleMaskColorRef, 1);
            visibleMaskSubpass.pDepthAttachment = &visibleMaskDepthRef;
            visibleMaskSubpass.pResolveAttachments = &visibleMaskResolveRef;

            RenderPassSpec visibleMaskPassSpec{};
            visibleMaskPassSpec.Name = "EditorOutlineVisibleMaskPass";
            visibleMaskPassSpec.Attachments = visibleMaskAttachments;
            visibleMaskPassSpec.SubPasses = Span32(&visibleMaskSubpass, 1);
            m_VisibleMaskRenderPass = Renderer::CreateRenderPass(visibleMaskPassSpec);

            const TextureViewHandle visibleMaskFbAttachments[] = {
                Renderer::GetTextureView(m_VisibleMaskMSAATexture.GetHandle(), TextureViewType::RenderTarget),
                Renderer::GetTextureView(m_VisibleMaskResolvedTexture.GetHandle(), TextureViewType::RenderTarget),
                Renderer::GetTextureView(m_SceneViewportPanel.GetDepthAttachment(), TextureViewType::DepthStencil)
            };

            FrameBufferSpec visibleMaskFbSpec;
            visibleMaskFbSpec.RenderPassHandle = m_VisibleMaskRenderPass.GetHandle();
            visibleMaskFbSpec.Attachments = visibleMaskFbAttachments;
            visibleMaskFbSpec.Size = {1, 1};
            m_VisibleMaskFrameBuffer = Renderer::CreateFrameBuffer(visibleMaskFbSpec);

            GraphicsPipelineStateCreateInfo visibleMaskPsoCI{};
            visibleMaskPsoCI.Name = "EditorOutlineVisibleMask";
            visibleMaskPsoCI.GraphicsPipeline.RenderPass = m_VisibleMaskRenderPass.GetHandle();

            visibleMaskPsoCI.VertexShader = pass->Pipelines.front().VertexShader;
            visibleMaskPsoCI.FragmentShader = pass->Pipelines.front().FragmentShader;

            // No input layout, vertex shader generates positions
            LayoutElementBuilder<5> layoutBuilder{
                LayoutElement{0, 0, ValueType::Float3},
                LayoutElement{1, 0, ValueType::Float3},
                LayoutElement{2, 0, ValueType::Float3},
                LayoutElement{4, 0, ValueType::Float3},
                LayoutElement{3, 0, ValueType::Float2}
            };

            visibleMaskPsoCI.GraphicsPipeline.InputLayout.LayoutElements = layoutBuilder;

            visibleMaskPsoCI.GraphicsPipeline.RasterizerSpec.CullMode = CullMode::Back;
            visibleMaskPsoCI.GraphicsPipeline.RasterizerSpec.FrontCounterClockwise = true;

            // No depth
            visibleMaskPsoCI.GraphicsPipeline.DepthStencilSpec.DepthEnable = true;
            visibleMaskPsoCI.GraphicsPipeline.DepthStencilSpec.DepthWriteEnable = false;
            visibleMaskPsoCI.GraphicsPipeline.DepthStencilSpec.DepthFunc = ComparisonFunc::LessEqual;

            visibleMaskPsoCI.GraphicsPipeline.SampleSpec.Count = SampleCount::x4;

            SmallVec<ShaderResourceVariableSpec, 2> vars = {
                {"global", ShaderType::VertexAndFragment, ShaderResourceVariableType::Static},
                {"Instances", ShaderType::VertexAndFragment, ShaderResourceVariableType::Mutable},
            };

            visibleMaskPsoCI.Spec.ResourceLayout.Variables = vars;

            m_VisibleMaskPSO = Renderer::CreatePipelineState(visibleMaskPsoCI);

            Renderer::BindStaticVariableByName(
                m_VisibleMaskPSO.GetHandle(),
                ShaderType::Vertex,
                "global",
                m_WorldRenderer.GetGlobalBuffer()
            );

            m_VisibleMaskSRB = Renderer::CreateShaderResourceBinding(m_VisibleMaskPSO.GetHandle(), true);

            Renderer::BindVariableByName(
                ShaderType::Vertex,
                m_VisibleMaskSRB.GetHandle(),
                "Instances",
                m_WorldRenderer.GetInstancesBufferView()
            );
        }
    }

    struct OutlineSettings {
        pfloat4 Color;
        pfloat4 ViewportSizeAndThickness;
    };

    void SceneWorkspace::CreateOutlineCompositeResources() {
        GpuBufferSpec uboSpec;
        uboSpec.Name = "OutlineSettings";
        uboSpec.Size = sizeof(OutlineSettings);
        uboSpec.Usage = Usage::Default;
        uboSpec.BindFlags = Bind::UniformBuffer;

        m_OutlineSettingsUB = Renderer::CreateBuffer(uboSpec, {});

        // Composite Pass
        RenderPassAttachmentSpec compositeAttachment;
        compositeAttachment.Format = ImageFormat::RGBA8UNorm;
        compositeAttachment.SampleCount = 1;
        compositeAttachment.LoadOp = AttachmentLoadOp::Load; // reads+writes scene color
        compositeAttachment.StoreOp = AttachmentStoreOp::Store;
        compositeAttachment.InitialState = ResourceState::RenderTarget;
        compositeAttachment.FinalState = ResourceState::ShaderResource;

        AttachmentReference compositeColorRef{ 0, ResourceState::RenderTarget };

        SubPassSpec compositeSubpass{};
        compositeSubpass.RenderTargetAttachments = Span32(&compositeColorRef, 1);

        RenderPassSpec compositePassSpec{};
        compositePassSpec.Name = "OutlineCompositePass";
        compositePassSpec.Attachments = Span32(&compositeAttachment, 1);
        compositePassSpec.SubPasses = Span32(&compositeSubpass, 1);
        m_CompositeRenderPass = Renderer::CreateRenderPass(compositePassSpec);

        TextureViewHandle compositeColorView = Renderer::GetTextureView(
            m_SceneViewportPanel.GetSceneColorTexture(),
            TextureViewType::RenderTarget
        );

        FrameBufferSpec fbDesc;
        fbDesc.Name = "SelectionCompositeFB";
        fbDesc.RenderPassHandle = m_CompositeRenderPass.GetHandle();
        fbDesc.Attachments = Span32(&compositeColorView, 1);
        fbDesc.Size = { 1, 1 };

        m_CompositeFrameBuffer = Renderer::CreateFrameBuffer(fbDesc);

        AssetMetadata selectedCompositeMetadata;
        selectedCompositeMetadata.FilePath = "Assets/shaders/SelectedOutlineComposite.slang";
        selectedCompositeMetadata.Handle = AssetID("deadbeef-dead-beef-dead-beefdeadbeef");
        selectedCompositeMetadata.Type = GraphicsShader::GetStaticType();

        GraphicsShader* shader = GetCompositeShader();

        ShaderImporter::Cook(selectedCompositeMetadata);
        ShaderImporter::Import(shader, selectedCompositeMetadata);

        const GraphicsShaderPass* pass = shader->GetShaderPass("GBuffer");

        GraphicsPipelineStateCreateInfo compositePsoCI{};
        compositePsoCI.Name = "OutlineComposite";
        compositePsoCI.GraphicsPipeline.RenderPass = m_CompositeRenderPass.GetHandle();

        compositePsoCI.VertexShader = pass->Pipelines.front().VertexShader;
        compositePsoCI.FragmentShader = pass->Pipelines.front().FragmentShader;

        // No input layout, vertex shader generates positions
        compositePsoCI.GraphicsPipeline.InputLayout.LayoutElements = {};

        compositePsoCI.GraphicsPipeline.RasterizerSpec.CullMode = CullMode::None; // no culling on fullscreen tri

        // No depth
        compositePsoCI.GraphicsPipeline.DepthStencilSpec.DepthEnable = false;

        compositePsoCI.GraphicsPipeline.BlendSpec.RenderTargets[0].BlendEnable = true;
        compositePsoCI.GraphicsPipeline.BlendSpec.RenderTargets[0].SrcBlend = BlendFactor::SrcAlpha;
        compositePsoCI.GraphicsPipeline.BlendSpec.RenderTargets[0].DestBlend = BlendFactor::InvSrcAlpha;
        compositePsoCI.GraphicsPipeline.BlendSpec.RenderTargets[0].BlendOp = BlendOperation::Add;

        SmallVec<ShaderResourceVariableSpec, 2> vars = {
            {"Settings", ShaderType::Fragment, ShaderResourceVariableType::Static},
            {"FullMask", ShaderType::Fragment, ShaderResourceVariableType::Dynamic},
            {"VisibleMask", ShaderType::Fragment, ShaderResourceVariableType::Dynamic},
        };

        compositePsoCI.Spec.ResourceLayout.Variables = vars;

        SamplerSpec samplerSpec;
        samplerSpec.WrapU = WrapMode::Clamp;
        samplerSpec.WrapV = WrapMode::Clamp;
        samplerSpec.MinFilter = FilterMode::Linear;
        samplerSpec.MagFilter = FilterMode::Linear;
        samplerSpec.MipFilter = FilterMode::Linear;

        ImmutableSamplerSpec selectionMaskSamplers[2];
        selectionMaskSamplers[0].SamplerOrTextureName = "FullMask";
        selectionMaskSamplers[0].Specification = samplerSpec;
        selectionMaskSamplers[0].ShaderStages = ShaderType::Fragment;

        selectionMaskSamplers[1].SamplerOrTextureName = "VisibleMask";
        selectionMaskSamplers[1].Specification = samplerSpec;
        selectionMaskSamplers[1].ShaderStages = ShaderType::Fragment;

        compositePsoCI.Spec.ResourceLayout.ImmutableSamplers = selectionMaskSamplers;

        m_CompositePSO = Renderer::CreatePipelineState(compositePsoCI);

        Renderer::BindStaticVariableByName(
            m_CompositePSO.GetHandle(),
            ShaderType::Fragment,
            "Settings",
            m_OutlineSettingsUB.GetHandle()
        );

        m_CompositeSRB = Renderer::CreateShaderResourceBinding(m_CompositePSO.GetHandle(), true);
    }

    void SceneWorkspace::CompositePass() {
        OutlineSettings settings = {
            .Color = Color{ 1.0, 0.62, 0.0, 1.0 },
            .ViewportSizeAndThickness = float4(m_SceneViewportPanel.GetViewportSize(), 3.0f, 0.0f),
        };

        Renderer::UpdateBuffer(m_OutlineSettingsUB.GetHandle(), 0, std::as_bytes(Span(&settings, 1)));

        // Bind ID buffer into composite SRB
        Renderer::BindVariableByName(
            ShaderType::Fragment,
            m_CompositeSRB.GetHandle(),
            "FullMask",
            Renderer::GetTextureView(m_FullMaskResolvedTexture.GetHandle(), TextureViewType::ShaderResource)
        );

        Renderer::BindVariableByName(
            ShaderType::Fragment,
            m_CompositeSRB.GetHandle(),
            "VisibleMask",
            Renderer::GetTextureView(m_VisibleMaskResolvedTexture.GetHandle(), TextureViewType::ShaderResource)
        );

        Renderer::BindPipelineState(m_CompositePSO.GetHandle());
        Renderer::CommitShaderResources(m_CompositeSRB.GetHandle(), ResourceStateTransitionMode::Transition);

        BeginRenderPassAttribs passAttribs{};
        passAttribs.RenderPassHandle = m_CompositeRenderPass.GetHandle();
        passAttribs.FrameBufferHandle = m_CompositeFrameBuffer.GetHandle();

        // LoadOp = Load, so no clear value needed
        Renderer::BeginRenderPass(passAttribs);

        // No vertex buffer, shader generates the triangle from SV_VertexID
        DrawAttribs draw{};
        draw.NumVertices = 3;
        Renderer::Draw(draw);

        Renderer::EndRenderPass();
    }

    void SceneWorkspace::RunMaskPass() {
        static Vec<InstanceData> maskInstances;
        maskInstances.clear();

        if (m_SelectedEntity.IsValid() && m_SelectedEntity.Has<WorldTransform>() && m_SelectedEntity.Has<MeshRenderer>()) {
            maskInstances.push_back(InstanceData {
                .Transform = m_SelectedEntity.Get<WorldTransform>().Value,
                .MaterialId = 0,
                .EntityIndex = 0
            });
        }

        if (maskInstances.empty()) {
            return;
        }

        void* mapped;
        Renderer::Map(m_WorldRenderer.GetInstancesGpuBuffer(), MapType::Write, MapFlags::Discard, mapped);
        memcpy(mapped, maskInstances.data(), sizeof(InstanceData) * maskInstances.size());
        Renderer::Unmap(m_WorldRenderer.GetInstancesGpuBuffer(), MapType::Write);

        BeginRenderPassAttribs passAttribs{};

        ClearValue clearValues[2];
        clearValues[0].Format = ImageFormat::R8UNorm;
        clearValues[0].Color = {0};

        clearValues[1] = {};

        passAttribs.ClearColors = clearValues;

        const Mesh* mesh = m_SelectedEntity.Get<MeshRenderer>().Mesh.TryGet();

        DrawIndexedAttribs attribs;
        attribs.Flags = DrawFlags::VerifyAll;
        attribs.IndexType = ValueType::UInt16;
        attribs.NumIndices = mesh->GetIndices().size();
        attribs.NumInstances = 1;
        attribs.FirstInstanceLocation = 0;

        // VISIBLE MASK PASS
        {
            passAttribs.RenderPassHandle = m_VisibleMaskRenderPass.GetHandle();
            passAttribs.FrameBufferHandle = m_VisibleMaskFrameBuffer.GetHandle();

            Renderer::BindPipelineState(m_VisibleMaskPSO.GetHandle());
            Renderer::CommitShaderResources(m_VisibleMaskSRB.GetHandle(), ResourceStateTransitionMode::Transition);

            Renderer::BeginRenderPass(passAttribs);

            Renderer::BindVertexBuffer(mesh->GetVertexBuffer(), 0);
            Renderer::BindIndexBuffer(mesh->GetIndexBuffer());

            Renderer::DrawIndexed(attribs);

            Renderer::EndRenderPass();
        }

        Renderer::Map(m_WorldRenderer.GetInstancesGpuBuffer(), MapType::Write, MapFlags::Discard, mapped);
        memcpy(mapped, maskInstances.data(), sizeof(InstanceData) * maskInstances.size());
        Renderer::Unmap(m_WorldRenderer.GetInstancesGpuBuffer(), MapType::Write);

        // FULL MASK PASS
        {
            passAttribs.RenderPassHandle = m_FullMaskRenderPass.GetHandle();
            passAttribs.FrameBufferHandle = m_FullMaskFrameBuffer.GetHandle();

            Renderer::BindPipelineState(m_FullMaskPSO.GetHandle());
            Renderer::CommitShaderResources(m_FullMaskSRB.GetHandle(), ResourceStateTransitionMode::Transition);


            Renderer::BeginRenderPass(passAttribs);

            Renderer::BindVertexBuffer(mesh->GetVertexBuffer(), 0);
            Renderer::BindIndexBuffer(mesh->GetIndexBuffer());

            Renderer::DrawIndexed(attribs);

            Renderer::EndRenderPass();
        }
   }
}

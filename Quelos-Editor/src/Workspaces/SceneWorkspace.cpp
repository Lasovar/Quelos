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

                Renderer::TextureResize(m_MaskTexture.GetHandle(), size.x, size.y);
                Renderer::TextureResize(m_MaskResolvedTexture.GetHandle(), size.x, size.y);
                Renderer::TextureResize(m_MaskDepthTexture.GetHandle(), size.x, size.y);
                Renderer::FrameBufferResize(m_MaskFrameBuffer.GetHandle(), size.x, size.y);

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

            if (m_SceneViewportPanel.SelectRequest().IsRequested() || m_SelectedEntity.IsValid()) {
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
            }

            if (m_SceneViewportPanel.SelectRequest().Resolve()) {
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
        // MSAA mask
        TextureSpecification maskSpec;
        maskSpec.Format = ImageFormat::R8UNorm;
        maskSpec.SampleCount = SampleCount::x4;
        maskSpec.BindFlags = Bind::RenderTarget;
        m_MaskTexture = Renderer::CreateTexture(maskSpec);

        // Resolved 1x mask, used by composite shader
        TextureSpecification maskResolvedSpec;
        maskResolvedSpec.Format = ImageFormat::R8UNorm;
        maskResolvedSpec.SampleCount = SampleCount::x1;
        maskResolvedSpec.BindFlags = Bind::RenderTarget | Bind::ShaderResource;
        m_MaskResolvedTexture = Renderer::CreateTexture(maskResolvedSpec);

        TextureSpecification msaaDepthSpec;
        msaaDepthSpec.Format = ImageFormat::DEPTH32Float;
        msaaDepthSpec.SamplerWrap = WrapMode::Repeat;

        msaaDepthSpec.BindFlags = Bind::DepthStencil;
        msaaDepthSpec.SampleCount = SampleCount::x4;

        m_MaskDepthTexture = Renderer::CreateTexture(msaaDepthSpec);

        RenderPassAttachmentSpec attachments[3];

        attachments[0].Format = ImageFormat::R8UNorm;
        attachments[0].SampleCount = 4;
        attachments[0].LoadOp = AttachmentLoadOp::Clear;
        attachments[0].StoreOp = AttachmentStoreOp::Store;
        attachments[0].InitialState = ResourceState::RenderTarget;
        attachments[0].FinalState = ResourceState::RenderTarget;

        attachments[1].Format = ImageFormat::R8UNorm;
        attachments[1].SampleCount = 1;
        attachments[1].LoadOp = AttachmentLoadOp::Clear;
        attachments[1].StoreOp = AttachmentStoreOp::Store;
        attachments[1].InitialState = ResourceState::ResolveDest;
        attachments[1].FinalState = ResourceState::ShaderResource;

        // SHARED scene depth, read-only, load existing values, don't clear/write
        attachments[2].Format = ImageFormat::DEPTH32Float;
        attachments[2].SampleCount = 4;
        attachments[2].LoadOp = AttachmentLoadOp::Clear;
        attachments[2].StoreOp = AttachmentStoreOp::Discard;
        attachments[2].InitialState = ResourceState::DepthWrite;
        attachments[2].FinalState = ResourceState::DepthWrite;

        AttachmentReference colorRef { 0, ResourceState::RenderTarget };
        AttachmentReference resolveRef{ 1, ResourceState::ResolveDest };
        AttachmentReference depthRef{ 2, ResourceState::DepthWrite };

        SubPassSpec maskSubpass{};
        maskSubpass.RenderTargetAttachments = Span32(&colorRef, 1);
        maskSubpass.pDepthAttachment = &depthRef;
        maskSubpass.pResolveAttachments = &resolveRef;

        RenderPassSpec maskPassSpec{};
        maskPassSpec.Name = "OutlineCompositePass";
        maskPassSpec.Attachments = attachments;
        maskPassSpec.SubPasses = Span32(&maskSubpass, 1);
        m_MaskRenderPass = Renderer::CreateRenderPass(maskPassSpec);

        const TextureViewHandle fbAttachments[] = {
            Renderer::GetTextureView(m_MaskTexture.GetHandle(), TextureViewType::RenderTarget),
            Renderer::GetTextureView(m_MaskResolvedTexture.GetHandle(), TextureViewType::RenderTarget),
            Renderer::GetTextureView(m_MaskDepthTexture.GetHandle(), TextureViewType::DepthStencil)
        };

        FrameBufferSpec fbDesc;
        fbDesc.RenderPassHandle = m_MaskRenderPass.GetHandle();
        fbDesc.Attachments = fbAttachments;
        fbDesc.Size = {1, 1};
        m_MaskFrameBuffer = Renderer::CreateFrameBuffer(fbDesc);

        AssetMetadata selectedMaskMetadata;
        selectedMaskMetadata.FilePath = "Assets/shaders/SelectedOutlineMask.slang";
        selectedMaskMetadata.Handle = AssetID("deaddead-beaf-beef-dead-deadbeafbeef");
        selectedMaskMetadata.Type = GraphicsShader::GetStaticType();

        GraphicsShader* shader = GetMaskShader();

        ShaderImporter::Cook(selectedMaskMetadata);
        ShaderImporter::Import(shader, selectedMaskMetadata);

        GraphicsPipelineStateCreateInfo psoCI{};
        psoCI.Name = "EditorOutlineMask";
        psoCI.GraphicsPipeline.RenderPass = m_MaskRenderPass.GetHandle();

        psoCI.VertexShader = shader->GetVertexShaderHandle();
        psoCI.FragmentShader = shader->GetFragmentShaderHandle();

        // No input layout, vertex shader generates positions
        LayoutElementBuilder<4> layoutBuilder{
            LayoutElement{0, 0, ValueType::Float3},
            LayoutElement{1, 0, ValueType::Float3},
            LayoutElement{2, 0, ValueType::Float3},
            LayoutElement{3, 0, ValueType::Float2}
        };

        psoCI.GraphicsPipeline.InputLayout.LayoutElements = layoutBuilder;

        psoCI.GraphicsPipeline.RasterizerSpec.CullMode = CullMode::Back;
        psoCI.GraphicsPipeline.RasterizerSpec.FrontCounterClockwise = true;

        // No depth
        psoCI.GraphicsPipeline.DepthStencilSpec.DepthEnable = true;
        psoCI.GraphicsPipeline.DepthStencilSpec.DepthWriteEnable = true;
        psoCI.GraphicsPipeline.SampleSpec.Count = SampleCount::x4;

        SmallVec<ShaderResourceVariableSpec, 2> vars = {
            {"global", ShaderType::VertexAndFragment, ShaderResourceVariableType::Static},
            {"Instances", ShaderType::VertexAndFragment, ShaderResourceVariableType::Mutable},
        };

        psoCI.Spec.ResourceLayout.Variables = vars;

        m_MaskPSO = Renderer::CreatePipelineState(psoCI);

        Renderer::BindStaticVariableByName(
            m_MaskPSO.GetHandle(),
            ShaderType::Vertex,
            "global",
            m_WorldRenderer.GetGlobalBuffer()
        );

        m_MaskSRB = Renderer::CreateShaderResourceBinding(m_MaskPSO.GetHandle(), true);

        Renderer::BindVariableByName(
            ShaderType::Vertex,
            m_MaskSRB.GetHandle(),
            "Instances",
            m_WorldRenderer.GetInstancesBufferView()
        );
    }

    struct OutlineSettings {
        pfloat4 Color;
        float Thickness;
        pfloat2 ViewportSize;
        float Padding;
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

        GraphicsPipelineStateCreateInfo compositePsoCI{};
        compositePsoCI.Name = "OutlineComposite";
        compositePsoCI.GraphicsPipeline.RenderPass = m_CompositeRenderPass.GetHandle();

        compositePsoCI.VertexShader = shader->GetVertexShaderHandle();
        compositePsoCI.FragmentShader = shader->GetFragmentShaderHandle();

        // No input layout, vertex shader generates positions
        compositePsoCI.GraphicsPipeline.InputLayout.LayoutElements = {};

        compositePsoCI.GraphicsPipeline.RasterizerSpec.CullMode = CullMode::None; // no culling on fullscreen tri

        // No depth
        compositePsoCI.GraphicsPipeline.DepthStencilSpec.DepthEnable = false;

        SmallVec<ShaderResourceVariableSpec, 2> vars = {
            {"Settings", ShaderType::Fragment, ShaderResourceVariableType::Static},
            {"SelectionMask", ShaderType::Fragment, ShaderResourceVariableType::Dynamic},
        };

        compositePsoCI.Spec.ResourceLayout.Variables = vars;

        SamplerSpec samplerSpec;
        samplerSpec.WrapU = WrapMode::Clamp;
        samplerSpec.WrapV = WrapMode::Clamp;
        samplerSpec.MinFilter = FilterMode::Linear;
        samplerSpec.MagFilter = FilterMode::Linear;
        samplerSpec.MipFilter = FilterMode::Linear;

        ImmutableSamplerSpec selectionMaskSampler;
        selectionMaskSampler.SamplerOrTextureName = "SelectionMask";
        selectionMaskSampler.Specification = samplerSpec;
        selectionMaskSampler.ShaderStages = ShaderType::Fragment;

        compositePsoCI.Spec.ResourceLayout.ImmutableSamplers = { &selectionMaskSampler, 1 };

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
            .Color = Color::Red(),
            .Thickness = 3.0f,
            .ViewportSize = m_SceneViewportPanel.GetViewportSize(),
            .Padding = 0.0f
        };

        Renderer::UpdateBuffer(m_OutlineSettingsUB.GetHandle(), 0, std::as_bytes(Span(&settings, 1)));

        // Bind ID buffer into composite SRB
        Renderer::BindVariableByName(
            ShaderType::Fragment,
            m_CompositeSRB.GetHandle(),
            "SelectionMask",
            Renderer::GetTextureView(m_MaskResolvedTexture.GetHandle(), TextureViewType::ShaderResource)
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
        passAttribs.RenderPassHandle = m_MaskRenderPass.GetHandle();
        passAttribs.FrameBufferHandle = m_MaskFrameBuffer.GetHandle();

        ClearValue clearValues[3];
        clearValues[0].Format = ImageFormat::R8UNorm;
        clearValues[0].Color = {0};

        clearValues[1] = {};

        clearValues[2].Format = ImageFormat::DEPTH32Float;
        clearValues[2].DepthStencil.Depth = 1.0;

        passAttribs.ClearColors = clearValues;

        Renderer::BeginRenderPass(passAttribs);

        Renderer::BindPipelineState(m_MaskPSO.GetHandle());
        Renderer::CommitShaderResources(m_MaskSRB.GetHandle(), ResourceStateTransitionMode::None);

        const Mesh* mesh = m_SelectedEntity.Get<MeshRenderer>().Mesh.TryGet();

        Renderer::BindVertexBuffer(mesh->GetVertexBuffer(), 0);
        Renderer::BindIndexBuffer(mesh->GetIndexBuffer());

        DrawIndexedAttribs attribs;
        attribs.Flags = DrawFlags::VerifyAll;
        attribs.IndexType = ValueType::UInt16;
        attribs.NumIndices = mesh->GetIndices().size();
        attribs.NumInstances = 1;
        attribs.FirstInstanceLocation = 0;

        Renderer::DrawIndexed(attribs);

        Renderer::EndRenderPass();
    }
}

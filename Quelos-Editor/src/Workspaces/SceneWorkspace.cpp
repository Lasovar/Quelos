#include "qspch.h"
#include "SceneWorkspace.h"

#include "EditorLayer.h"
#include "imgui_internal.h"
#include "AssetManagement/AssetImporters/SceneImporter.h"
#include "AssetManagement/AssetImporters/ShaderImporter.h"
#include "Quelos/ImGui/imgui_zoomable_image.h"
#include "Quelos/Scenes/SceneSnapshot.h"

using namespace magic_enum::bitwise_operators;

namespace QuelosEditor {
    SceneWorkspace::SceneWorkspace(UndoSystem& undoSystem, const AssetMetadata& assetMetadata)
        : Workspace(std::string(FS::Stem(assetMetadata.FilePath)), undoSystem),
          m_GameViewportPanel("Game View", *this, 1, 1),
          m_SceneViewportPanel("Scene View", *this, 1, 1),
          m_InspectorPanel(*this, undoSystem),
          m_EntityHierarchyPanel(*this, undoSystem)
    {
        m_WorkspaceNameHash = assetMetadata.ID.ToString();
        ComponentRegistry::RegisterBuiltinTypes(m_EditorWorld);
        ComponentRegistry::RegisterBuiltinTypes(m_RuntimeWorld);

        m_EditorWorld.set_threads(6);
        m_RuntimeWorld.set_threads(6);

        m_RuntimeWorld.entity<SimulationPhase>()
                                   .add(flecs::Phase)
                                   .depends_on(flecs::OnUpdate);

        m_EditorWorld.import<LocalToWorldTransformSystem>();
        m_RuntimeWorld.import<LocalToWorldTransformSystem>();

        m_EditorScene = SceneImporter::ImportScene(assetMetadata.ID, assetMetadata, m_EditorWorld);
        m_ActiveScene = m_EditorScene;

        m_InspectorPanel.SetScene(m_ActiveScene);
        m_EntityHierarchyPanel.SetScene(m_ActiveScene);

        // World renderer setup (a few of these need to be a render pass after the system for custom render pass is setup)
        m_WorldRenderer.SetWorld(m_EditorWorld);

        const auto& assetManager = SharedAs<EditorAssetManager>(Project::GetAssetManager());

        const AssetMetadata* computeMetadata = assetManager->GetAssetMetadata("Assets/shaders/DepthReduction.slang");
        ShaderImporter::Load(LoadContext(*computeMetadata, GetDepthReductionCompute()));
        m_WorldRenderer.SetDepthReductionCompute(GetDepthReductionCompute());

        const AssetMetadata* shadowDepthMetadata = assetManager->GetAssetMetadata("Assets/shaders/ShadowDepth.slang");
        ShaderImporter::Load(LoadContext(*shadowDepthMetadata, GetShadowDepthShader()));
        m_WorldRenderer.SetShadowDepthShader(GetShadowDepthShader());
        m_WorldRenderer.SetDepthPrepassShader(GetShadowDepthShader());

        const AssetMetadata* shadowMaskMetadata = assetManager->GetAssetMetadata("Assets/shaders/ShadowMask.slang");
        ShaderImporter::Load(LoadContext(*shadowMaskMetadata, GetShadowMaskShader()));
        m_WorldRenderer.SetShadowMaskShader(GetShadowMaskShader());

        const AssetMetadata* toneMappingShaderMetadata = assetManager->GetAssetMetadata("Assets/shaders/tonemapping.slang");
        ShaderImporter::Load(LoadContext(*toneMappingShaderMetadata, GetToneMappingShader()));
        m_WorldRenderer.SetToneMappingShader(GetToneMappingShader());

        m_GameViewportPanel.SetWorldRendererView(m_WorldRenderer.CreateView("GameView", { 1, 1 }));
        m_SceneViewportPanel.SetWorldRendererView(m_WorldRenderer.CreateView("SceneView", { 1, 1 }));

        m_WorkspaceID = ImHashStr((m_ActiveScene->GetName() + "_SceneDockspace").c_str());
        m_DefaultWorkspaceDockingCondition = ImGuiCond_Appearing;
        m_ShouldDock = true;

        m_EditorCamera = EditorCamera(60.0f, 1.0f, 0.1f, 1000.0f);

        m_SceneSerializer = SceneSerializer(m_EditorScene, assetMetadata.FilePath);
        m_UndoSystem.AddSceneSerializer(m_EditorScene, &m_SceneSerializer);

        m_ContentBrowserPanel.Init();

        // TODO: Editor Scene view renderer

        TextureSpec idDesc;
        idDesc.Name = "ActorIDBuffer";
        idDesc.Type = TextureType::Texture2D;
        idDesc.Width = 1;
        idDesc.Height = 1;
        idDesc.Format = TextureFormat::R32UInt;
        idDesc.BindFlags = Bind::RenderTarget;
        idDesc.Usage = Usage::Default;

        m_IDTexture = Renderer::CreateTexture(idDesc);

        TextureSpec stagingDesc = idDesc;
        stagingDesc.Usage = Usage::Staging;
        stagingDesc.BindFlags = Bind::None;
        stagingDesc.CpuAccessFlags = CpuAccess::Read;
        m_IDStagingTexture = Renderer::CreateTexture(stagingDesc);

        TextureSpec idDepthSpec;
        idDepthSpec.Name = "ActorIDDepth";
        idDepthSpec.Type = TextureType::Texture2D;
        idDepthSpec.Width = 1;
        idDepthSpec.Height = 1;
        idDepthSpec.Format = TextureFormat::Depth32Float;
        idDepthSpec.BindFlags = Bind::DepthStencil;
        idDepthSpec.Usage = Usage::Default;
        m_IDDepthTexture = Renderer::CreateTexture(idDepthSpec);

        RenderPassAttachmentSpec idAttachments[2];
        idAttachments[0].Format = TextureFormat::R32UInt;
        idAttachments[0].SampleCount = 1;
        idAttachments[0].LoadOp = AttachmentLoadOp::Clear;
        idAttachments[0].StoreOp = AttachmentStoreOp::Store;
        idAttachments[0].InitialState = ResourceState::RenderTarget;
        idAttachments[0].FinalState = ResourceState::CopySource;

        idAttachments[1].Format = TextureFormat::Depth32Float;
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
            Renderer::TextureGetDefaultView(m_IDTexture.GetHandle(), TextureViewType::RenderTarget),
            Renderer::TextureGetDefaultView(m_IDDepthTexture.GetHandle(), TextureViewType::DepthStencil),
        };

        FrameBufferSpec fbDesc;
        fbDesc.Name = "ActorIDFrameBuffer";
        fbDesc.RenderPassHandle = m_ActorIDRenderPass.GetHandle();
        fbDesc.Attachments = idFbAttachments;
        fbDesc.Size = { .Width = 1, .Height = 1 };

        m_IDFrameBuffer = Renderer::CreateFrameBuffer(fbDesc);

        const AssetMetadata* entitySelectionMetadata = assetManager->GetAssetMetadata("Assets/shaders/EntitySelection.slang");
        ShaderImporter::Load(LoadContext(*entitySelectionMetadata, GetIDShader()));

        GraphicsShader* shader = GetIDShader();
        GraphicsPipelineStateCreateInfo pipelineStateCreateInfo;
        pipelineStateCreateInfo.Name = shader->GetName();

        pipelineStateCreateInfo.GraphicsPipeline.RenderPass = m_ActorIDRenderPass.GetHandle();
        pipelineStateCreateInfo.GraphicsPipeline.RasterizerSpec.CullMode = CullMode::Back;
        pipelineStateCreateInfo.GraphicsPipeline.RasterizerSpec.FrontCounterClockwise = true;
        pipelineStateCreateInfo.GraphicsPipeline.DepthStencilSpec.DepthEnable = true;
        pipelineStateCreateInfo.GraphicsPipeline.DepthStencilSpec.DepthFunc = ComparisonFunc::GreaterEqual;
        pipelineStateCreateInfo.GraphicsPipeline.DepthStencilSpec.DepthWriteEnable = true;

        pipelineStateCreateInfo.GraphicsPipeline.SampleSpec.Count = SampleCount::x1;

        pipelineStateCreateInfo.GraphicsPipeline.InputLayout.LayoutElements = Vertex::GetLayoutElements();

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
            m_WorldRenderer.GetInstancesBufferView(),
            SetShaderResourceFlag::None
        );

        FenceSpec fenceSpec;
        fenceSpec.Name = "PickFence";
        m_PickFence = Renderer::CreateFence(fenceSpec);

        CreateOutlineMaskResources();
        CreateOutlineCompositeResources();

        m_RuntimeWorld.system<LocalTransform&>()
                      .kind<SimulationPhase>()
                      .with<MeshRenderer>()
                      .each([](const flecs::iter& it, size_t, LocalTransform& transform) {
                          transform.Rotation = math::mul(transform.Rotation, quaternion::rotation_y(it.delta_time()));
                      });

        const AssetMetadata* skyboxShaderMetadata = assetManager->GetAssetMetadata("Assets/shaders/skybox.slang");
        ShaderImporter::Load(LoadContext(*skyboxShaderMetadata, GetSkyboxShader()));

        auto* hdrMetadata = assetManager->GetAssetMetadata("Assets/Textures/HDRIs/qwantani_dusk_2_puresky_4k.hdr");
        m_SkyboxTexture = AssetRef<Texture>(hdrMetadata->ID);

        m_WorldRenderer.SetSkyboxShader(GetSkyboxShader(), m_SkyboxTexture);

        const AssetMetadata* dfgLutMetadata = assetManager->GetAssetMetadata("Assets/shaders/dfg_lut.slang");
        ShaderImporter::Load(LoadContext(*dfgLutMetadata, GetDfgLutShader()));
        m_WorldRenderer.SetDfgLutComputeShader(GetDfgLutShader());

        const AssetMetadata* irradianceCubeMetadata = assetManager->GetAssetMetadata("Assets/shaders/irradiance_cube.slang");
        ShaderImporter::Load(LoadContext(*irradianceCubeMetadata, GetIrradianceCubeShader()));
        m_WorldRenderer.SetIrradianceCubeShader(GetIrradianceCubeShader());

        const AssetMetadata* prefilteredCubeMetadata = assetManager->GetAssetMetadata("Assets/shaders/prefiltered_cube.slang");
        ShaderImporter::Load(LoadContext(*prefilteredCubeMetadata, GetPrefilteredCubeShader()));
        m_WorldRenderer.SetPrefilteredCubeShader(GetPrefilteredCubeShader());
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

        float dt = deltaTime;
        if (m_SceneState == SceneState::Play) {
            if (Optional<bool> pauseRequest = m_PauseRequest.Resolve()) {
                if (*pauseRequest) {
                    m_RuntimeWorld.entity<SimulationPhase>().disable();
                } else {
                    m_RuntimeWorld.entity<SimulationPhase>().enable();
                }

                m_IsScenePaused = *pauseRequest;
            }

            if (m_StepRequest.IsRequested()) {
                m_RuntimeWorld.entity<SimulationPhase>().enable();
                dt = 1 / 30.0f;
            }
        }

        m_ActiveScene->Tick(dt);

        if (m_StepRequest.Resolve()) {
            m_RuntimeWorld.entity<SimulationPhase>().disable();
        }

        if (m_SceneViewportPanel.ShouldDraw() || m_GameViewportPanel.ShouldDraw()) {
            m_WorldRenderer.Begin();
        }

        if (m_SceneViewportPanel.ShouldDraw()) {
            if (m_PickRequest.IsRequested() && Renderer::FenceGetCompletedValue(m_PickFence.GetHandle()) >= m_PickFenceValue) {
                m_PickRequest = false;

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

                    if (pickedId > 0 && pickedId - 1 < m_PickIds.size()) {
                        SetSelectEntity(m_PickIds[pickedId - 1].Target<PipelineOf>());
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

                RecreateOutlineCompositeSRB();

                Renderer::FrameBufferResize(m_CompositeFrameBuffer.GetHandle(), size.x, size.y);
            }

            RenderViewParams sceneViewParams {
                .Projection = m_EditorCamera.GetProjection(),
                .View = m_EditorCamera.GetViewMatrix(),
                .CameraPosition = m_EditorCamera.GetPosition(),
                .SceneColorClear = {0.2667f, 0.2000f, 0.3333f, 1.0000f},
                .NearClip = m_EditorCamera.GetNearClip(),
                .FarClip = m_EditorCamera.GetFarClip(),
                .EnvironmentIntensity = m_EnvironmentIntensity,
                .ExposureEV = m_ExposureEV
            };

            m_WorldRenderer.Render(
                m_SceneViewportPanel.GetWorldRendererView(),
                sceneViewParams
            );

            if (m_SceneViewportPanel.SelectRequest().Resolve()) {
                ClearValue idClearValues[2];
                idClearValues[0].Format = TextureFormat::R32UInt;

                float clearAsFloat = 0.0f;
                idClearValues[0].Color = {clearAsFloat};

                idClearValues[1].Format = TextureFormat::Depth32Float;
                idClearValues[1].DepthStencil.Depth = 0.0f;

                BeginRenderPassAttribs attribs;
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
                                .EntityIndex = j + 1
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

                Renderer::EnqueueSignal(m_PickFence.GetHandle(), ++m_PickFenceValue);

                m_PickRequest = true;
            }

            if (m_SelectedEntity.IsValid()) {
                SelectedEntityOutline();
            }
        }

        if (m_GameViewportPanel.ShouldDraw()) {
            if (m_GameViewportPanel.ResizeIfNeeded()) {
                m_ActiveScene->OnViewportResized(m_GameViewportPanel.GetViewportSize());
            }

            RenderViewParams renderViewParams = m_ActiveScene->GetRenderViewParams();
            renderViewParams.EnvironmentIntensity = m_EnvironmentIntensity;
            renderViewParams.ExposureEV = m_ExposureEV;

            m_WorldRenderer.Render(m_GameViewportPanel.GetWorldRendererView(), renderViewParams);
        }

        if (m_SceneViewportPanel.ShouldDraw() || m_GameViewportPanel.ShouldDraw()) {
            m_WorldRenderer.End();
        }
    }

    void SceneWorkspace::WorkspaceContents() {
        m_GameViewportPanel.OnImGuiRender(m_WorkspaceID, m_WorkspaceClass);

        m_SceneViewportPanel.SetFrame(m_SelectedEntity, m_EditorCamera.GetViewMatrix(), m_EditorCamera.GetProjection());
        m_SceneViewportPanel.OnImGuiRender(m_WorkspaceID, m_WorkspaceClass);

        if (UI::Begin("Environment", m_WorkspaceID, m_WorkspaceClass)) {
            ImGui::DragFloat("Environment Intensity", &m_EnvironmentIntensity, 0.01f);
            ImGui::SliderFloat("ExposureEV", &m_ExposureEV, -6, 6);
        }
        UI::End();

        /*
        if (ImGui::Begin("DFG LUT")) {
            if (m_SceneViewportPanel.ShouldDraw()) {
                float4 uv(0.0f, 0.0f, 1.0f, 1.0f);

                ImGuiImage::Zoomable(
                    Renderer::TextureGetNativeHandle(m_WorldRenderer.GetDfgLutSRV()),
                    {128, 128},
                    {uv.x, uv.y},
                    {uv.z, uv.w}
                );
            }
        }
        ImGui::End();
        */

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
        SetScenePaused(false);
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
        m_ActiveScene = CreateShared<Scene>(m_RuntimeWorld);
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
        const auto& assetManager = SharedAs<EditorAssetManager>(Project::GetAssetManager());
        const AssetMetadata* selectedMaskMetadata = assetManager->GetAssetMetadata("Assets/shaders/SelectedOutlineMask.slang");

        GraphicsShader* shader = GetMaskShader();
        ShaderImporter::Load(LoadContext(*selectedMaskMetadata, shader));

        const GraphicsShaderPass* pass = shader->GetShaderPass("SelectedOutlineMask");

        // FULL MASK
        {
            TextureSpec fullMaskMSAASpec;
            fullMaskMSAASpec.Format = TextureFormat::R8UNorm;
            fullMaskMSAASpec.SampleCount = SampleCount::x4;
            fullMaskMSAASpec.BindFlags = Bind::RenderTarget;
            m_FullMaskMSAATexture = Renderer::CreateTexture(fullMaskMSAASpec);

            TextureSpec fullMaskResolvedSpec;
            fullMaskResolvedSpec.Format = TextureFormat::R8UNorm;
            fullMaskResolvedSpec.SampleCount = SampleCount::x1;
            fullMaskResolvedSpec.BindFlags = Bind::RenderTarget | Bind::ShaderResource;

            m_FullMaskResolvedTexture = Renderer::CreateTexture(fullMaskResolvedSpec);
            m_FullMaskSRV = Renderer::TextureGetDefaultView(
                m_FullMaskResolvedTexture.GetHandle(),
                TextureViewType::ShaderResource
            );

            RenderPassAttachmentSpec fullMaskAttachments[2];

            fullMaskAttachments[0].Format = TextureFormat::R8UNorm;
            fullMaskAttachments[0].SampleCount = 4;
            fullMaskAttachments[0].LoadOp = AttachmentLoadOp::Clear;
            fullMaskAttachments[0].StoreOp = AttachmentStoreOp::Store;
            fullMaskAttachments[0].InitialState = ResourceState::RenderTarget;
            fullMaskAttachments[0].FinalState = ResourceState::RenderTarget;

            fullMaskAttachments[1].Format = TextureFormat::R8UNorm;
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
                Renderer::TextureGetDefaultView(m_FullMaskMSAATexture.GetHandle(), TextureViewType::RenderTarget),
                Renderer::TextureGetDefaultView(m_FullMaskResolvedTexture.GetHandle(), TextureViewType::RenderTarget)
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

            fullMaskPsoCI.GraphicsPipeline.InputLayout.LayoutElements = Vertex::GetLayoutElements();

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
                m_WorldRenderer.GetInstancesBufferView(),
                SetShaderResourceFlag::None
            );
        }

        // VISIBLE MASK
        {
            // MSAA mask
            TextureSpec visibleMaskSpec;
            visibleMaskSpec.Format = TextureFormat::R8UNorm;
            visibleMaskSpec.SampleCount = SampleCount::x4;
            visibleMaskSpec.BindFlags = Bind::RenderTarget;
            m_VisibleMaskMSAATexture = Renderer::CreateTexture(visibleMaskSpec);

            // Resolved 1x mask, used by composite shader
            TextureSpec visibleMaskResolvedSpec;
            visibleMaskResolvedSpec.Format = TextureFormat::R8UNorm;
            visibleMaskResolvedSpec.SampleCount = SampleCount::x1;
            visibleMaskResolvedSpec.BindFlags = Bind::RenderTarget | Bind::ShaderResource;

            m_VisibleMaskResolvedTexture = Renderer::CreateTexture(visibleMaskResolvedSpec);
            m_VisibleMaskSRV = Renderer::TextureGetDefaultView(
                m_VisibleMaskResolvedTexture.GetHandle(),
                TextureViewType::ShaderResource
            );

            RenderPassAttachmentSpec visibleMaskAttachments[3];

            visibleMaskAttachments[0].Format = TextureFormat::R8UNorm;
            visibleMaskAttachments[0].SampleCount = 4;
            visibleMaskAttachments[0].LoadOp = AttachmentLoadOp::Clear;
            visibleMaskAttachments[0].StoreOp = AttachmentStoreOp::Store;
            visibleMaskAttachments[0].InitialState = ResourceState::RenderTarget;
            visibleMaskAttachments[0].FinalState = ResourceState::RenderTarget;

            visibleMaskAttachments[1].Format = TextureFormat::R8UNorm;
            visibleMaskAttachments[1].SampleCount = 1;
            visibleMaskAttachments[1].LoadOp = AttachmentLoadOp::Clear;
            visibleMaskAttachments[1].StoreOp = AttachmentStoreOp::Store;
            visibleMaskAttachments[1].InitialState = ResourceState::ResolveDest;
            visibleMaskAttachments[1].FinalState = ResourceState::ShaderResource;

            // SHARED scene depth, read-only, load existing values, don't clear/write
            visibleMaskAttachments[2].Format = TextureFormat::Depth32Float;
            visibleMaskAttachments[2].SampleCount = 4;
            visibleMaskAttachments[2].LoadOp = AttachmentLoadOp::Load;
            visibleMaskAttachments[2].StoreOp = AttachmentStoreOp::Store;
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
                Renderer::TextureGetDefaultView(m_VisibleMaskMSAATexture.GetHandle(), TextureViewType::RenderTarget),
                Renderer::TextureGetDefaultView(m_VisibleMaskResolvedTexture.GetHandle(), TextureViewType::RenderTarget),
                m_SceneViewportPanel.GetWorldRendererView()->SceneDepthDSV
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

            visibleMaskPsoCI.GraphicsPipeline.InputLayout.LayoutElements = Vertex::GetLayoutElements();

            visibleMaskPsoCI.GraphicsPipeline.RasterizerSpec.CullMode = CullMode::Back;
            visibleMaskPsoCI.GraphicsPipeline.RasterizerSpec.FrontCounterClockwise = true;

            // No depth
            visibleMaskPsoCI.GraphicsPipeline.DepthStencilSpec.DepthEnable = true;
            visibleMaskPsoCI.GraphicsPipeline.DepthStencilSpec.DepthWriteEnable = false;
            visibleMaskPsoCI.GraphicsPipeline.DepthStencilSpec.DepthFunc = ComparisonFunc::GreaterEqual;

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
                m_WorldRenderer.GetInstancesBufferView(),
                SetShaderResourceFlag::None
            );
        }
    }

    struct alignas(16) OutlineSettings {
        // ReSharper disable once CppDeclaratorNeverUsed
        pfloat4 Color;
        // ReSharper disable once CppDeclaratorNeverUsed
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
        compositeAttachment.Format = TextureFormat::RGBA8UNorm;
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

        TextureViewHandle compositeColorView = m_SceneViewportPanel.GetWorldRendererView()->FinalSceneColorRTV;

        FrameBufferSpec fbDesc;
        fbDesc.Name = "SelectionCompositeFB";
        fbDesc.RenderPassHandle = m_CompositeRenderPass.GetHandle();
        fbDesc.Attachments = Span32(&compositeColorView, 1);
        fbDesc.Size = { 1, 1 };

        m_CompositeFrameBuffer = Renderer::CreateFrameBuffer(fbDesc);

        const auto& assetManager = SharedAs<EditorAssetManager>(Project::GetAssetManager());
        const AssetMetadata* selectedCompositeMetadata = assetManager->GetAssetMetadata("Assets/shaders/SelectedOutlineComposite.slang");

        GraphicsShader* shader = GetCompositeShader();
        ShaderImporter::Load(LoadContext(*selectedCompositeMetadata, shader));

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

        RenderTargetBlendSpec& renderTargetBlendSpec = compositePsoCI.GraphicsPipeline.BlendSpec.RenderTargets[0];
        renderTargetBlendSpec.BlendEnable = true;
        renderTargetBlendSpec.SrcBlend = BlendFactor::SrcAlpha;
        renderTargetBlendSpec.DestBlend = BlendFactor::InvSrcAlpha;
        renderTargetBlendSpec.BlendOp = BlendOperation::Add;
        renderTargetBlendSpec.SrcBlendAlpha = BlendFactor::Zero;
        renderTargetBlendSpec.DestBlendAlpha = BlendFactor::One;
        renderTargetBlendSpec.BlendOpAlpha = BlendOperation::Add;

        SmallVec<ShaderResourceVariableSpec, 3> vars = {
            {"Settings", ShaderType::Fragment, ShaderResourceVariableType::Static},
            {"FullMask", ShaderType::Fragment, ShaderResourceVariableType::Mutable},
            {"VisibleMask", ShaderType::Fragment, ShaderResourceVariableType::Mutable},
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

        RecreateOutlineCompositeSRB();
    }

    void SceneWorkspace::RecreateOutlineCompositeSRB() {
        m_CompositeSRB = Renderer::CreateShaderResourceBinding(m_CompositePSO.GetHandle(), true);

        Renderer::BindVariableByName(
            ShaderType::Fragment,
            m_CompositeSRB.GetHandle(),
            "VisibleMask",
            m_VisibleMaskSRV,
            SetShaderResourceFlag::None
        );

        Renderer::BindVariableByName(
            ShaderType::Fragment,
            m_CompositeSRB.GetHandle(),
            "FullMask",
            m_FullMaskSRV,
            SetShaderResourceFlag::None
        );
    }

    void SceneWorkspace::SelectedEntityOutline() const {
        if (!m_SelectedEntity.Has<WorldTransform>() || !m_SelectedEntity.Has<MeshRenderer>()) {
            return;
        }

        const Optional<Ref<Mesh>> meshOptional = m_SelectedEntity.Get<MeshRenderer>().Mesh.TryGet();
        if (!meshOptional.has_value()) {
            return;
        }

        const auto& mesh = meshOptional.value().get();

        static Vec<InstanceData> maskInstances(Allocator::Persistent);

        maskInstances.clear();
        maskInstances.push_back(InstanceData {
            .Transform = m_SelectedEntity.Get<WorldTransform>().Value,
            .MaterialId = 0,
            .EntityIndex = 0
        });

        if (maskInstances.empty()) {
            return;
        }

        void* mapped;
        Renderer::Map(m_WorldRenderer.GetInstancesGpuBuffer(), MapType::Write, MapFlags::Discard, mapped);
        memcpy(mapped, maskInstances.data(), sizeof(InstanceData) * maskInstances.size());
        Renderer::Unmap(m_WorldRenderer.GetInstancesGpuBuffer(), MapType::Write);

        BeginRenderPassAttribs passAttribs{};

        ClearValue clearValues[2];
        clearValues[0].Format = TextureFormat::R8UNorm;
        clearValues[0].Color = {0};

        clearValues[1] = {};

        passAttribs.ClearColors = clearValues;

        DrawIndexedAttribs attribs;
        attribs.Flags = DrawFlags::VerifyAll;
        attribs.IndexType = ValueType::UInt16;
        attribs.NumIndices = mesh.GetIndices().size();
        attribs.NumInstances = 1;
        attribs.FirstInstanceLocation = 0;

        // VISIBLE MASK PASS
        {
            passAttribs.RenderPassHandle = m_VisibleMaskRenderPass.GetHandle();
            passAttribs.FrameBufferHandle = m_VisibleMaskFrameBuffer.GetHandle();

            Renderer::BindPipelineState(m_VisibleMaskPSO.GetHandle());
            Renderer::CommitShaderResources(m_VisibleMaskSRB.GetHandle(), ResourceStateTransitionMode::Transition);

            Renderer::BeginRenderPass(passAttribs);

            Renderer::BindVertexBuffer(mesh.GetVertexBuffer(), 0);
            Renderer::BindIndexBuffer(mesh.GetIndexBuffer());

            Renderer::DrawIndexed(attribs);

            Renderer::EndRenderPass();
        }

        // FULL MASK PASS
        {
            passAttribs.RenderPassHandle = m_FullMaskRenderPass.GetHandle();
            passAttribs.FrameBufferHandle = m_FullMaskFrameBuffer.GetHandle();

            Renderer::BindPipelineState(m_FullMaskPSO.GetHandle());
            Renderer::CommitShaderResources(m_FullMaskSRB.GetHandle(), ResourceStateTransitionMode::Transition);

            Renderer::BeginRenderPass(passAttribs);

            Renderer::BindVertexBuffer(mesh.GetVertexBuffer(), 0);
            Renderer::BindIndexBuffer(mesh.GetIndexBuffer());

            Renderer::DrawIndexed(attribs);

            Renderer::EndRenderPass();
        }

        // Composite Pass
        {
            OutlineSettings settings = {
                .Color = Color{ 1.0, 0.62, 0.0, 1.0 },
                .ViewportSizeAndThickness = float4(m_SceneViewportPanel.GetViewportSize(), 2.0f, 0.0f),
            };

            Renderer::UpdateBuffer(m_OutlineSettingsUB.GetHandle(), 0, std::as_bytes(Span64(&settings, 1)));

            // Bind ID buffer into composite SRB

            Renderer::BindPipelineState(m_CompositePSO.GetHandle());
            Renderer::CommitShaderResources(m_CompositeSRB.GetHandle(), ResourceStateTransitionMode::Transition);

            BeginRenderPassAttribs compositePassAttribs{};
            compositePassAttribs.RenderPassHandle = m_CompositeRenderPass.GetHandle();
            compositePassAttribs.FrameBufferHandle = m_CompositeFrameBuffer.GetHandle();

            // LoadOp = Load, so no clear value needed
            Renderer::BeginRenderPass(compositePassAttribs);

            // No vertex buffer, shader generates the triangle from SV_VertexID
            DrawAttribs draw{};
            draw.NumVertices = 3;
            Renderer::Draw(draw);

            Renderer::EndRenderPass();
        }
   }
}

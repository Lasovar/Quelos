#pragma once

#include "Workspace.h"

#include "Quelos/Renderer/EditorCamera.h"
#include "Quelos/Scenes/Scene.h"

#include "Panels/ContentBrowserPanel.h"
#include "Panels/EntityHierarchyPanel.h"
#include "Panels/EntityInspectorPanel.h"
#include "Panels/ViewportPanel.h"
#include "Quelos/Scenes/SceneSnapshot.h"
#include "Quelos/Utility/Request.h"
#include "SceneWorkspace/GameViewport.h"

#include "Workspaces/SceneWorkspace/SceneViewport.h"

namespace QuelosEditor {
    using namespace Quelos;

    using SelectionCallback = std::function<void(const Entity&)>;

    enum class SceneState {
        Edit = 0, Play
    };

    class SceneWorkspace : public Workspace {
    public:
        explicit SceneWorkspace(UndoSystem& undoSystem, const AssetMetadata& assetMetadata);

        void SetSelectEntity(Entity entity);

        void Tick(float deltaTime) override;
        void WorkspaceContents() override;

        void OnEvent(Event& event) override;

        GraphicsShader* GetIDShader() { return std::launder(reinterpret_cast<GraphicsShader*>(m_IDShaderStorage)); }
        [[nodiscard]] const GraphicsShader* GetIDShader() const { return std::launder(reinterpret_cast<const GraphicsShader*>(m_IDShaderStorage)); }

        GraphicsShader* GetCompositeShader() { return std::launder(reinterpret_cast<GraphicsShader*>(m_CompositeShaderStorage)); }
        [[nodiscard]] const GraphicsShader* GetCompositeShader() const { return std::launder(reinterpret_cast<const GraphicsShader*>(m_CompositeShaderStorage)); }

        GraphicsShader* GetMaskShader() { return std::launder(reinterpret_cast<GraphicsShader*>(m_MaskShaderStorage)); }
        [[nodiscard]] const GraphicsShader* GetMaskShader() const { return std::launder(reinterpret_cast<const GraphicsShader*>(m_MaskShaderStorage)); }

        const Ref<Scene>& GetScene() { return m_ActiveScene; }
        [[nodiscard]] Entity GetSelectedEntity() const { return m_SelectedEntity; }

        void ScenePlay();
        void SceneStop();

        void Init();

        [[nodiscard]] SceneState GetSceneState() const { return m_SceneState; }
        [[nodiscard]] bool IsScenePaused() const { return m_ScenePaused; }
        void ToggleScenePaused() { m_ScenePaused = !m_ScenePaused; }
        void SetScenePaused(const bool value) { m_ScenePaused = value; }
        void SetSceneStep(const bool value) { m_SceneStep = value; }

        void OnEntitySelectionChanged(SelectionCallback callback) {
            m_OnSelectionChangedCallbacks.push_back(std::move(callback));
        }
    private:
        void OnScenePlay();
        void OnSceneStop();

        void CreateOutlineMaskResources();
        void CreateOutlineCompositeResources();
        void CompositePass();
        void RunMaskPass();

    private:
        flecs::world m_EditorWorld;
        flecs::world m_RuntimeWorld;
        Vec<SelectionCallback> m_OnSelectionChangedCallbacks;

        SceneState m_SceneState = SceneState::Edit;
        bool m_ScenePaused = false;
        bool m_SceneStep = false;

        Request m_PlayRequest;
        Request m_StopRequest;

        SceneSnapshot m_SceneSnapshot;
        Ref<Scene> m_EditorScene;
        Ref<Scene> m_ActiveScene;

        WorldRenderer m_WorldRenderer;

        SceneSerializer m_SceneSerializer;

        GameViewport m_GameViewportPanel;
        SceneViewport m_SceneViewportPanel;

        Entity m_SelectedEntity;
        Request m_PickRequest = false;
        Vec<Entity> m_PickIds;

        EntityInspectorPanel m_InspectorPanel;
        EntityHierarchyPanel m_EntityHierarchyPanel;

        ContentBrowserPanel m_ContentBrowserPanel;

        EditorCamera m_EditorCamera;

        bool m_CtrlKey: 1 = false;
        bool m_ShiftKey: 1 = false;

        ResourceRef<Texture> m_IDTexture;
        ResourceRef<Texture> m_IDStagingTexture;
        ResourceRef<Texture> m_IDDepthTexture;
        ResourceRef<RenderPass> m_ActorIDRenderPass;
        ResourceRef<FrameBuffer> m_IDFrameBuffer;
        ResourceRef<PipelineStateObject> m_IDPSO;
        ResourceRef<ShaderResourceBinding> m_IDSRB;

        alignas(GraphicsShader) byte m_IDShaderStorage[sizeof(GraphicsShader)]{};

        ResourceRef<GpuBuffer> m_OutlineSettingsUB;
        ResourceRef<RenderPass> m_CompositeRenderPass;
        ResourceRef<FrameBuffer> m_CompositeFrameBuffer;
        ResourceRef<PipelineStateObject> m_CompositePSO;
        ResourceRef<ShaderResourceBinding> m_CompositeSRB;

        alignas(GraphicsShader) byte m_CompositeShaderStorage[sizeof(GraphicsShader)]{};

        ResourceRef<Texture> m_FullMaskMSAATexture;
        ResourceRef<Texture> m_FullMaskResolvedTexture;
        ResourceRef<FrameBuffer> m_FullMaskFrameBuffer;
        ResourceRef<RenderPass> m_FullMaskRenderPass;
        ResourceRef<PipelineStateObject> m_FullMaskPSO;
        ResourceRef<ShaderResourceBinding> m_FullMaskSRB;

        ResourceRef<Texture> m_VisibleMaskMSAATexture;
        ResourceRef<Texture> m_VisibleMaskResolvedTexture;
        ResourceRef<FrameBuffer> m_VisibleMaskFrameBuffer;
        ResourceRef<RenderPass> m_VisibleMaskRenderPass;
        ResourceRef<PipelineStateObject> m_VisibleMaskPSO;
        ResourceRef<ShaderResourceBinding> m_VisibleMaskSRB;

        alignas(GraphicsShader) byte m_MaskShaderStorage[sizeof(GraphicsShader)]{};
    };
}

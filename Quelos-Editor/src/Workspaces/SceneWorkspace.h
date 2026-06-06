#pragma once

#include "Workspace.h"

#include "Quelos/Renderer/EditorCamera.h"
#include "Quelos/Scenes/Scene.h"

#include "Panels/ContentBrowserPanel.h"
#include "Panels/EntityHierarchyPanel.h"
#include "Panels/EntityInspectorPanel.h"
#include "Panels/ViewportPanel.h"

#include "Workspaces/SceneWorkspace/SceneViewport.h"

namespace QuelosEditor {
    using namespace Quelos;

    using SelectionCallback = std::function<void(const Entity&)>;
    class SceneWorkspace : public Workspace {
    public:
        explicit SceneWorkspace(UndoSystem& undoSystem, const AssetMetadata& assetMetadata);

        void SetSelectEntity(Entity entity);

        void Tick(float deltaTime) override;
        void WorkspaceContents() override;

        void OnEvent(Event& event) override;

        GraphicsShader* GetIDShader() { return std::launder(reinterpret_cast<GraphicsShader*>(m_IDShaderStorage)); }
        const GraphicsShader* GetIDShader() const { return std::launder(reinterpret_cast<const GraphicsShader*>(m_IDShaderStorage)); }
        const Ref<Scene>& GetScene() { return m_Scene; }
        Entity GetSelectedEntity() const { return m_SelectedEntity; }

        void OnEntitySelectionChanged(SelectionCallback callback) {
            m_OnSelectionChangedCallbacks.push_back(std::move(callback));
        }

    private:
        Vec<SelectionCallback> m_OnSelectionChangedCallbacks;

        Ref<Scene> m_Scene;
        SceneSerializer m_SceneSerializer;

        ViewportPanel m_GameViewportPanel;
        SceneViewport m_SceneViewportPanel;

        Entity m_SelectedEntity;

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
    };
}

#include "qspch.h"
#include "SceneWorkspace.h"

#include "imgui.h"
#include "imgui_internal.h"

#include "Quelos/Renderer/Renderer.h"

namespace QuelosEditor {
    SceneWorkspace::SceneWorkspace(const Ref<Scene>& scene, UndoSystem& undoSystem) : Workspace(scene->GetName()),
        m_Scene(scene), m_UndoSystem(undoSystem), m_InspectorPanel(EntityInspectorPanel(scene, undoSystem)),
        m_EntityHierarchyPanel(scene, undoSystem)
    {
        m_EntityHierarchyPanel.AddListenerOnEntitySelected([this](const Actor& actor) {
            m_InspectorPanel.SetSelectedEntity(actor);
        });

        m_WorkspaceID = ImHashStr((m_Scene->GetName() + "_Dockspace").c_str());

        m_GameViewportPanel = ViewportPanel("Game View", 0, 1, 1);
        m_SceneViewportPanel = ViewportPanel("Scene View", 1, 1, 1);

        m_EditorCamera = EditorCamera(60.0f, 1.0f, 0.1f, 1000.0f);

        const AssetMetadata* metadata = RefAs<EditorAssetManager>(
            Project::GetAssetManager())->GetAssetMetadata(m_Scene->GetAssetID()
        );

        if (!metadata) {
            QS_CORE_ERROR_TAG(
                "SceneWorkspace",
                "Failed to get scene metadata for scene '{}'({})",
                scene->GetName(),
                scene->GetAssetID().ToString()
            );

            return;
        }

        m_SceneSerializer = SceneSerializer(m_Scene, metadata->FilePath);
        m_UndoSystem.AddSceneSerializer(m_Scene, &m_SceneSerializer);
        m_ContentBrowserPanel.Init();
    }

    void SceneWorkspace::SelectEntity(const Entity entity) {
        m_InspectorPanel.SetSelectedEntity(entity);
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
                const glm::vec2 size = m_SceneViewportPanel.GetViewportSize();
                m_EditorCamera.SetViewportSize(size.x, size.y);
            }

            Renderer::StartSceneRender(
                m_SceneViewportPanel.GetFrameBuffer(),
                m_EditorCamera.GetViewMatrix(),
                m_EditorCamera.GetProjection()
            );

            m_Scene->Render(m_SceneViewportPanel.GetFrameBuffer()->GetViewID());
        }

        if (m_GameViewportPanel.ShouldDraw()) {
            if (m_GameViewportPanel.ResizeIfNeeded()) {
                m_Scene->OnViewportResized(m_GameViewportPanel.GetViewportSize());
            }

            m_Scene->StartRender(m_GameViewportPanel.GetFrameBuffer());
            m_Scene->Render(m_GameViewportPanel.GetFrameBuffer()->GetViewID());
        }
    }

    void SceneWorkspace::WorkspaceContents() {
        m_GameViewportPanel.OnImGuiRender(m_WorkspaceID, m_WorkspaceClass);
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
}

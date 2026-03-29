#include "qspch.h"
#include "SceneWorkspace.h"

#include "imgui.h"
#include "imgui_internal.h"

#include "Quelos/Renderer/Renderer.h"

namespace QuelosEditor {
    SceneWorkspace::SceneWorkspace(const Ref<Scene>& scene, UndoSystem& undoSystem)
        : m_Scene(scene), m_UndoSystem(undoSystem), m_InspectorPanel(EntityInspectorPanel(scene, undoSystem)),
          m_EntityHierarchyPanel(scene, undoSystem) {
        m_EntityHierarchyPanel.AddListenerOnEntitySelected([this](const Actor& actor) {
            m_InspectorPanel.SetSelectedEntity(actor);
        });

        m_SceneWorkspaceClass.ClassId = ImHashStr(scene->GetName().c_str());
        m_SceneWorkspaceClass.DockingAllowUnclassed = false;

        m_WorkspaceID = ImHashStr((m_Scene->GetName() + "_Dockspace").c_str());

        m_GameViewportPanel = ViewportPanel("Game View", 0, 1, 1);
        m_SceneViewportPanel = ViewportPanel("Scene View", 1, 1, 1);

        m_EditorCamera = EditorCamera(60.0f, 1.0f, 0.1f, 1000.0f);

        const AssetMetadata* metadata = RefAs<EditorAssetManager>(
            Project::GetAssetManager())->GetAssetMetadata(m_Scene->GetAssetHandle()
        );

        if (!metadata) {
            QS_CORE_ERROR_TAG(
                "SceneWorkspace",
                "Failed to get scene metadata for scene '{}'({})",
                scene->GetName(),
                scene->GetAssetHandle().ToString()
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

    void SceneWorkspace::OnImGuiRender(const ImGuiID dockspaceID) {
        // Force this workspace to live in the main central dock
        if (const ImGuiDockNode* node = ImGui::DockBuilderGetNode(dockspaceID); node && node->CentralNode) {
            ImGui::SetNextWindowDockID(node->CentralNode->ID, ImGuiCond_None);
        }

        constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;

        ImGui::Begin(m_Scene->GetName().c_str(), nullptr, flags);

        // Workspace-local dockspace
        ImGui::DockSpace(m_WorkspaceID, ImVec2(0, 0), ImGuiDockNodeFlags_None, &m_SceneWorkspaceClass);

        m_GameViewportPanel.OnImGuiRender(m_WorkspaceID, m_SceneWorkspaceClass);
        m_SceneViewportPanel.OnImGuiRender(m_WorkspaceID, m_SceneWorkspaceClass);
        m_EntityHierarchyPanel.OnImGuiRender(m_WorkspaceID, m_SceneWorkspaceClass);
        m_InspectorPanel.OnImGuiRender(m_WorkspaceID, m_SceneWorkspaceClass);
        m_ContentBrowserPanel.OnImGuiRender(m_WorkspaceID, m_SceneWorkspaceClass);

        ImGui::End();
    }

    void SceneWorkspace::OnEvent(Event& event) {
        if (m_SceneViewportPanel.IsViewportFocused() || m_SceneViewportPanel.IsViewportHovered()) {
            m_EditorCamera.OnEvent(event);
        }

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

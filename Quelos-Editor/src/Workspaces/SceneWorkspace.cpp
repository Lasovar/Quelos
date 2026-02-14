#include "qspch.h"
#include "SceneWorkspace.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "Quelos/Renderer/Renderer.h"

namespace Quelos {
    SceneWorkspace::SceneWorkspace() {
        m_GameViewportPanel = ViewportPanel("Game View", 0, 1, 1);
        m_SceneViewportPanel = ViewportPanel("Scene View", 1, 1, 1);

        m_SceneWorkspaceClass.ClassId = ImHashStr("SceneWorkspace");
        m_SceneWorkspaceClass.DockingAllowUnclassed = false;

        m_WorkspaceID = "SceneWorkspace_Dockspace";
        m_EditorCamera = EditorCamera(60.0f, 1.0f, 0.1f, 1000.0f);
    }

    void SceneWorkspace::Tick(const float deltaTime) {
        m_Scene->Tick(deltaTime);

        if (m_SceneViewportPanel.ShouldDraw()) {
            if (m_SceneViewportPanel.ResizeIfNeeded()) {
                const glm::vec2 size = m_SceneViewportPanel.GetViewportSize();
                m_EditorCamera.SetViewportSize(size.x, size.y);
                m_Scene->OnViewportResized(m_SceneViewportPanel.GetViewportSize());
            }

            m_Scene->StartRender(m_SceneViewportPanel.GetFrameBuffer());
            m_Scene->Render(m_SceneViewportPanel.GetFrameBuffer()->GetViewID());
        }

        if (m_GameViewportPanel.ShouldDraw()) {
            if (m_GameViewportPanel.ResizeIfNeeded()) {
                //m_Scene->OnViewportResized(m_GameViewportPanel.GetViewportSize());
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
        const ImGuiID workspaceDockId = ImGui::GetID(m_WorkspaceID.c_str());
        ImGui::DockSpace(workspaceDockId, ImVec2(0, 0), ImGuiDockNodeFlags_None, &m_SceneWorkspaceClass);

        m_GameViewportPanel.OnImGuiRender(workspaceDockId, m_SceneWorkspaceClass);

        m_SceneViewportPanel.OnImGuiRender(workspaceDockId, m_SceneWorkspaceClass);

        ImGui::End();
    }

    void SceneWorkspace::SetScene(const Ref<Scene>& scene) {
        m_Scene = scene;
        m_SceneWorkspaceClass.ClassId = ImHashStr(scene->GetName().c_str());
        m_WorkspaceID = m_Scene->GetName() + "_Dockspace";
    }
}

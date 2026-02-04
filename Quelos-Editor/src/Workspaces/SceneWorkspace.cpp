#include "qspch.h"
#include "SceneWorkspace.h"

#include "imgui.h"
#include "imgui_internal.h"

namespace Quelos {
    SceneWorkspace::SceneWorkspace() {
        m_ViewportPanel = ViewportPanel(1, 1);

        m_SceneWorkspaceClass.ClassId = ImHashStr("SceneWorkspace");
        m_SceneWorkspaceClass.DockingAllowUnclassed = false;

        m_WorkspaceID = "SceneWorkspace_Dockspace";
    }

    void SceneWorkspace::Tick(const float deltaTime) {
        if (!m_ViewportPanel.ShouldDraw()) {
            return;
        }

        m_ViewportPanel.ResizeIfNeeded();
        m_Scene->Tick(deltaTime);
        m_Scene->Render(0, m_ViewportPanel.GetFrameBuffer());
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

        m_ViewportPanel.OnImGuiRender(workspaceDockId, m_SceneWorkspaceClass);

        ImGui::End();
    }

    void SceneWorkspace::SetScene(const Ref<Scene>& scene) {
        m_Scene = scene;
        m_SceneWorkspaceClass.ClassId = ImHashStr(scene->GetName().c_str());
        m_WorkspaceID = m_Scene->GetName() + "_Dockspace";
    }
}

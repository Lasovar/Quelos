#include "qspch.h"
#include "SceneWorkspace.h"

#include "imgui.h"
#include "imgui_internal.h"

namespace Quelos {
    SceneWorkspace::SceneWorkspace() {
        m_ViewportPanel = ViewportPanel(1280, 720);

        m_SceneWorkspaceClass.ClassId = ImHashStr("SceneWorkspace");
        m_SceneWorkspaceClass.DockingAllowUnclassed = false;
    }

    void SceneWorkspace::Tick(const float deltaTime) {
        m_ViewportPanel.ResizeIfNeeded();

        m_Scene->Tick(deltaTime);
        m_Scene->Render(0, m_ViewportPanel.GetFrameBuffer());
    }

    void SceneWorkspace::OnImGuiRender(const ImGuiID dockspaceID) {
        // Force this workspace to live in the main central dock
        const ImGuiDockNode* node = ImGui::DockBuilderGetNode(dockspaceID);
        if (node && node->CentralNode) {
            ImGui::SetNextWindowDockID(node->CentralNode->ID, ImGuiCond_FirstUseEver);
        }

        constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;

        ImGui::Begin("Scene Workspace", nullptr, flags);

        // Workspace-local dockspace
        m_WorkspaceDockID = ImGui::GetID("SceneWorkspace_DockSpace");

        ImGui::DockSpace(m_WorkspaceDockID, ImVec2(0, 0), ImGuiDockNodeFlags_None, &m_SceneWorkspaceClass);

        m_ViewportPanel.OnImGuiRender(m_WorkspaceDockID, m_SceneWorkspaceClass);

        ImGui::End();
    }
}

#include "Workspace.h"

#include "imgui_internal.h"
#include "Quelos/ImGui/ImGuiUI.h"

namespace QuelosEditor {
    Workspace::Workspace(std::string workspaceName) : m_WorkspaceName(std::move(workspaceName)) {
        m_WorkspaceID = ImHashStr(UI::FormatTemp("{}_Dockspace", m_WorkspaceName));

        m_WorkspaceClass.ClassId = ImHashStr(m_WorkspaceName.c_str());
        m_WorkspaceClass.DockingAllowUnclassed = false;
    }

    void Workspace::OnImGuiRender(const ImGuiID dockspaceID) {
        // Force this workspace to live in the main central dock
        if (const ImGuiDockNode* node = ImGui::DockBuilderGetNode(dockspaceID); node && node->CentralNode) {
            ImGui::SetNextWindowDockID(node->CentralNode->ID, ImGuiCond_None);
        }

        constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;

        if (m_FocusRequest && m_FocusRequestFrame < ImGui::GetFrameCount() - 1) {
            ImGui::SetNextWindowFocus();
            m_FocusRequest = false;
        }

        if (ImGui::Begin(m_WorkspaceName.c_str(), &m_IsOpen, flags)) {
            // Workspace-local dockspace
            ImGui::DockSpace(m_WorkspaceID, ImVec2(0, 0), ImGuiDockNodeFlags_NoWindowMenuButton, &m_WorkspaceClass);

            WorkspaceContents();
        }

        ImGui::End();
    }
}

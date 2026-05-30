#include "Workspace.h"

#include "imgui_internal.h"
#include "Quelos/ImGui/ImGuiUI.h"

namespace QuelosEditor {
    Workspace::Workspace(std::string workspaceName, UndoSystem& undoSystem)
        : m_UndoSystem(undoSystem), m_WorkspaceName(std::move(workspaceName))
    {
        m_WorkspaceID = ImHashStr(FormatTemp("{}_Dockspace", m_WorkspaceName));

        m_WorkspaceClass.ClassId = ImHashStr(m_WorkspaceName.c_str());
        m_WorkspaceClass.DockingAllowUnclassed = false;
    }

    void Workspace::OnImGuiRender(const ImGuiWindowClass& dockspaceClass, const ImGuiID dockspaceID) {
        // Force this workspace to live in the main central dock
        const ImGuiDockNode* node = ImGui::DockBuilderGetNode(dockspaceID);
        if (m_ShouldDock && node && node->CentralNode) {
            ImGui::SetNextWindowDockID(node->CentralNode->ID, m_DefaultWorkspaceDockingCondition);
        }

        ImGui::SetNextWindowClass(&dockspaceClass);

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

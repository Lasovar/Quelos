#include "EditorLayer.h"
#include <imgui.h>

#include <Quelos/Scenes/Components.h>
#include <Quelos/Core/Log.h>

#include "imgui_internal.h"
#include "Quelos/Core/Base.h"

namespace Quelos {
    EditorLayer::EditorLayer() {
    }

    void EditorLayer::OnAttach() {
        m_DefaultScene = CreateRef<Scene>();
        m_SceneWorkspace = CreateRef<SceneWorkspace>();
        m_SceneWorkspace->SetScene(m_DefaultScene);

        m_Workspaces.push_back(m_SceneWorkspace);
    }

    void EditorLayer::Tick(const float deltaTime) {
        for (const auto& workspace : m_Workspaces) {
            workspace->Tick(deltaTime);
        }
    }

    void EditorLayer::ImGuiRender() {
        static ImGuiDockNodeFlags dockspace_flags =
            ImGuiDockNodeFlags_NoSplit |
            ImGuiDockNodeFlags_NoResize |
            ImGuiDockNodeFlags_NoDockingOverOther;

        static ImGuiWindowFlags window_flags =
            ImGuiWindowFlags_MenuBar |
            ImGuiWindowFlags_NoDocking |
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove;

        static ImGuiWindowClass s_MainEditorClass;
        s_MainEditorClass.ClassId = ImHashStr("MainEditor");
        s_MainEditorClass.DockingAllowUnclassed = false;

        const ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(vp->Pos);
        ImGui::SetNextWindowSize(vp->Size);
        ImGui::SetNextWindowViewport(vp->ID);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);

        ImGui::Begin("MainEditor", nullptr, window_flags);
        ImGui::PopStyleVar(2);

        ImGuiID globalDockspaceID = ImGui::GetID("GlobalWorkspaceDockSpace");
        ImGui::DockSpace(globalDockspaceID, ImVec2(0, 0), dockspace_flags, &s_MainEditorClass);

        static bool opt_fullscreen = true;
        static bool opt_padding = false;

        if (ImGuiDockNode* node = ImGui::DockBuilderGetNode(globalDockspaceID)) {
            // If the only one window... no undocking
            if (node->Windows.Size == 1) {
                node->LocalFlags |= ImGuiDockNodeFlags_NoUndocking;
            } else {
                node->LocalFlags &= ~ImGuiDockNodeFlags_NoUndocking;
            }
        }

        // Show demo options and help
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("Options")) {
                // Disabling fullscreen would allow the window to be moved to the front of other windows,
                // which we can't undo at the moment without finer window depth/z control.
                ImGui::MenuItem("Fullscreen", NULL, &opt_fullscreen);
                ImGui::MenuItem("Padding", NULL, &opt_padding);
                ImGui::Separator();

                if (ImGui::MenuItem("Flag: NoDockingOverCentralNode", "",
                                    (dockspace_flags & ImGuiDockNodeFlags_NoDockingOverCentralNode) != 0)) {
                    dockspace_flags ^= ImGuiDockNodeFlags_NoDockingOverCentralNode;
                }
                if (ImGui::MenuItem("Flag: NoDockingSplit", "",
                                    (dockspace_flags & ImGuiDockNodeFlags_NoDockingSplit) != 0)) {
                    dockspace_flags ^= ImGuiDockNodeFlags_NoDockingSplit;
                }
                if (ImGui::MenuItem("Flag: NoUndocking", "", (dockspace_flags & ImGuiDockNodeFlags_NoUndocking) != 0)) {
                    dockspace_flags ^= ImGuiDockNodeFlags_NoUndocking;
                }
                if (ImGui::MenuItem("Flag: NoResize", "", (dockspace_flags & ImGuiDockNodeFlags_NoResize) != 0)) {
                    dockspace_flags ^= ImGuiDockNodeFlags_NoResize;
                }
                if (ImGui::MenuItem("Flag: AutoHideTabBar", "",
                                    (dockspace_flags & ImGuiDockNodeFlags_AutoHideTabBar) != 0)) {
                    dockspace_flags ^= ImGuiDockNodeFlags_AutoHideTabBar;
                }
                if (ImGui::MenuItem("Flag: PassthruCentralNode", "",
                                    (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode) != 0, opt_fullscreen)) {
                    dockspace_flags ^= ImGuiDockNodeFlags_PassthruCentralNode;
                }
                ImGui::Separator();
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Help")) {
                ImGui::TextUnformatted(
                    "This demo has nothing to do with enabling docking!" "\n"
                    "This demo only demonstrate the use of ImGui::DockSpace() which allows you to manually\ncreate a docking node _within_ another window."
                    "\n"
                    "Most application can simply call ImGui::DockSpaceOverViewport() and be done with it.");
                ImGui::Separator();
                ImGui::TextUnformatted(
                    "When docking is enabled, you can ALWAYS dock MOST window into another! Try it now!" "\n"
                    "- Drag from window title bar or their tab to dock/undock." "\n"
                    "- Drag from window menu button (upper-left button) to undock an entire node (all windows)." "\n"
                    "- Hold SHIFT to disable docking (if io.ConfigDockingWithShift == false, default)" "\n"
                    "- Hold SHIFT to enable docking (if io.ConfigDockingWithShift == true)");
                ImGui::Separator();
                ImGui::TextUnformatted("More details:");
                ImGui::Bullet();
                ImGui::SameLine();
                ImGui::TextLinkOpenURL("Docking Wiki page", "https://github.com/ocornut/imgui/wiki/Docking");
                ImGui::BulletText("Read comments in ShowExampleAppDockSpace()");
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        ImGui::End();

        ImGui::ShowDemoWindow();

        for (const auto& workspace : m_Workspaces) {
            workspace->OnImGuiRender(globalDockspaceID);
        }
    }
}

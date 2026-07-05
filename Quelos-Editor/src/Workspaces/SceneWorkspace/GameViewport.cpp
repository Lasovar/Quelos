//
// Created by lasovar on 6/6/26.
//

#include "GameViewport.h"

#include "Workspaces/SceneWorkspace.h"
#include "UndoSystem.h"

namespace QuelosEditor {
    GameViewport::GameViewport(
        std::string name, SceneWorkspace& sceneWorkspace,
        const uint32_t width,
        const uint32_t height
    ) : ViewportPanel(std::move(name), width, height), m_SceneWorkspace(sceneWorkspace) {
    }

    void GameViewport::BeforeViewport() {
        constexpr float framePadding = 2.0f;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, framePadding));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0.0f, 0.0f });
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0);

        const auto& colors = ImGui::GetStyle().Colors;
        const ImVec4 hovered = colors[ImGuiCol_ButtonHovered];
        const ImVec4 active = colors[ImGuiCol_ButtonActive];

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(hovered.x, hovered.y, hovered.z, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(active.x, active.y, active.z, 0.5f));

        ImVec2 toolbarSize = { ImGui::GetContentRegionMax().x, ImGui::GetFrameHeight() + framePadding * 2.0f };
        if (ImGui::BeginChild("##toolbar", toolbarSize, ImGuiChildFlags_Borders,
                         ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar
                         | ImGuiWindowFlags_NoScrollWithMouse)) {
            float size = ImGui::GetFrameHeight();

            const SceneState sceneState = m_SceneWorkspace.GetSceneState();
            const char* icon = sceneState == SceneState::Edit
                                      ? ICON_FA_PLAY
                                      : ICON_FA_STOP;

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 4.0f, 0.0f });

            ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x * 0.5f - size * 2.0f);
            if (ImGui::Button(icon, {size, size})) {
                if (sceneState == SceneState::Edit) {
                    m_SceneWorkspace.ScenePlay();
                }
                else if (sceneState == SceneState::Play) {
                    m_SceneWorkspace.SceneStop();
                }
            }

            ImGui::SameLine();

            {
                bool pushed = false;
                if (m_SceneWorkspace.IsScenePaused()) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
                    pushed = true;
                }

                if (ImGui::Button(ICON_FA_PAUSE, {size, size})) {
                    m_SceneWorkspace.ToggleScenePaused();
                }

                if (pushed) {
                    ImGui::PopStyleColor();
                }
            }

            ImGui::SameLine();

            if (ImGui::Button(ICON_FA_STEP_FORWARD, {size, size})) {
                m_SceneWorkspace.SetScenePaused(true);
                m_SceneWorkspace.StepScene();
            }

            ImGui::PopStyleVar(); // ImGuiStyleVar_ItemSpacing

            ImGui::EndChild();
        }

        ImGui::PopStyleColor(3); // ImGuiCol_Button, ImGuiCol_ButtonHovered, ImGuiCol_ButtonActive

        ImGui::PopStyleVar(); // ImGuiStyleVar_ChildRounding
        ImGui::PopStyleVar(); // ImGuiStyleVar_FramePadding
        ImGui::PopStyleVar(); // ImGuiStyleVar_WindowPadding
    }
}

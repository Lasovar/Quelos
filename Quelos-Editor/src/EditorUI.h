#pragma once

#include "Quelos/ImGui/ImGuiUI.h"

namespace QuelosEditor {
    using namespace Quelos;

    namespace UI {
        using namespace Quelos::UI;

        inline bool Begin(std::string_view name, const ImGuiID dockspaceID, const ImGuiWindowClass& windowClass,
            bool* enabled = nullptr, const ImGuiWindowFlags flags = 0) {
            ImGui::SetNextWindowDockID(dockspaceID, ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowClass(&windowClass);

            return ImGui::Begin(FormatTemp("{}##{}", name, windowClass.ClassId), enabled, flags);
        }

        inline void End() {
            ImGui::End();
        }
    }
}

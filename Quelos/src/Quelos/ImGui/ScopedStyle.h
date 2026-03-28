#pragma once

#include "imgui.h"

namespace Quelos {
    struct ScopedStyleVar {
        ScopedStyleVar(const ImGuiStyleVar index, const ImVec2& value) {
            ImGui::PushStyleVar(index, value);
        }

        ScopedStyleVar(const ImGuiStyleVar index, const float value) {
            ImGui::PushStyleVar(index, value);
        }

        ~ScopedStyleVar() {
            ImGui::PopStyleVar();
        }
    };

    struct ScopedStyleVarX {
        ScopedStyleVarX(const ImGuiStyleVar index, const float value) {
            ImGui::PushStyleVarX(index, value);
        }

        ~ScopedStyleVarX() {
            ImGui::PopStyleVar();
        }
    };


    struct ScopedStyleVarY {
        ScopedStyleVarY(const ImGuiStyleVar index, const float value) {
            ImGui::PushStyleVarY(index, value);
        }

        ~ScopedStyleVarY() {
            ImGui::PopStyleVar();
        }
    };
}

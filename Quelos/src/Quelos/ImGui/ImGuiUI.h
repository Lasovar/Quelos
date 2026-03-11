#pragma once

#include <string>

#include "imgui.h"
#include "imgui_internal.h"

#include "glm/vec3.hpp"
#include "glm/gtc/type_ptr.hpp"

namespace Quelos::UI {
    inline std::string BeautifyLabel(std::string label) {
        if (label.rfind("m_", 0) == 0) {
            label.erase(0, 2);
        }
        else if (!label.empty() && label[0] == '_') {
            label.erase(0, 1);
        }

        if (!label.empty()) {
            label[0] = static_cast<char>(std::toupper(label[0]));
        }

        return label;
    }


    inline bool EditFloat(const std::string& label, float& value, float speed = 0.1f, float min = 0, float max = 0) {
        ImGui::PushID(label.c_str());

        ImGui::Columns(2, nullptr, false);

        ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() / 3.0f);
        ImGui::Text(BeautifyLabel(label).c_str());
        ImGui::NextColumn();

        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
        ImGui::BeginGroup();

        const bool used = ImGui::DragFloat("##f", &value, speed, min, max);

        ImGui::EndGroup();
        ImGui::PopItemWidth();

        ImGui::PopStyleVar();

        ImGui::Columns(1);

        ImGui::PopID();

        ImGui::Spacing();

        return used;
    }

    struct AxisColor {
        ImVec4 Base;
        ImVec4 Hover;
        ImVec4 Active;
    };

    consteval AxisColor GetAxisColor(const char axis) {
        switch (axis) {
        case 'X': return {{0.8f, 0.1f, 0.15f, 1}, {0.9f, 0.2f, 0.2f, 1}, {0.8f, 0.1f, 0.15f, 1}};
        case 'Y': return {{0.2f, 0.7f, 0.2f, 1}, {0.3f, 0.8f, 0.3f, 1}, {0.2f, 0.7f, 0.3f, 1}};
        case 'Z': return {{0.1f, 0.25f, 0.8f, 1}, {0.2f, 0.3f, 0.9f, 1}, {0.1f, 0.25f, 0.8f, 1}};
        case 'W': return {{0.65f, 0.5f, 0.1f, 1}, {0.8f, 0.65f, 0.2f, 1}, {0.6f, 0.45f, 0.1f, 1}};
        case 'Q': return {{0.6f, 0.3f, 0.9f, 1}, {0.7f, 0.4f, 1.0f, 1}, {0.6f, 0.3f, 0.9f, 1}};
        default: return {{0.5f, 0.5f, 0.5f, 1}, {0.6f, 0.6f, 0.6f, 1}, {0.5f, 0.5f, 0.5f, 1}};
        }
    }

    inline bool DrawAxis(
        const char* label,
        float& value,
        const float resetValue,
        const float speed,
        const float min,
        const float max,
        const float dragWidth,
        const ImVec2& buttonSize,
        const AxisColor& color,
        ImFont* boldFont
    ) {
        bool changed = false;

        ImGui::PushStyleColor(ImGuiCol_Button, color.Base);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color.Hover);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, color.Active);

        ImGui::PushFont(boldFont);
        if (ImGui::Button(label, buttonSize)) {
            value = resetValue;
            changed = true;
        }
        ImGui::PopFont();
        ImGui::PopStyleColor(3);

        ImGui::SameLine();

        ImGui::PushItemWidth(dragWidth);
        changed |= ImGui::DragFloat(
            std::format("##{}", label).c_str(),
            &value,
            speed,
            min,
            max,
            "%.2f"
        );
        ImGui::PopItemWidth();

        return changed;
    }

    template <TupleLike T>
    bool EditVectorN(
        const std::string& rawLabel,
        T& value,
        const std::array<AxisColor, 4> colors,
        const std::array<const char*, 4> axisLabels,
        const T resetValue,
        const float speed,
        const float min,
        const float max
    ) {
        const ImGuiIO& io = ImGui::GetIO();
        ImFont* boldFont = io.Fonts->Fonts[0];

        const std::string label = BeautifyLabel(rawLabel);
        bool changed = false;
        const uint32_t count = T::length();

        ImGui::PushID(label.c_str());
        ImGui::Columns(2, nullptr, false);

        ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() / 3.0f);
        ImGui::TextUnformatted(label.c_str());
        ImGui::NextColumn();

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {0, 0});

        const float totalWidth = ImGui::GetContentRegionAvail().x;
        const float spacing = ImGui::GetStyle().ItemSpacing.x;

        const float lineHeight = GImGui->FontSize + GImGui->Style.FramePadding.y * 2.0f;
        const ImVec2 buttonSize = {lineHeight + 3.0f, lineHeight};

        const float totalButtonsWidth = buttonSize.x * count;
        const float totalSpacingWidth = spacing * (count - 1);

        float dragWidth = (totalWidth - totalButtonsWidth - totalSpacingWidth) / count;

        ImGui::BeginGroup();

        for (int i = 0; i < count; i++) {
            if (i == count - 1) {
                dragWidth = ImGui::GetContentRegionAvail().x;
            }

            changed |= DrawAxis(
                axisLabels[i],
                value[i],
                resetValue[i],
                speed,
                min,
                max,
                dragWidth,
                buttonSize,
                colors[i],
                boldFont
            );

            if (i != count - 1) {
                ImGui::SameLine();
            }
        }

        ImGui::EndGroup();

        ImGui::PopStyleVar();
        ImGui::Columns(1);
        ImGui::PopID();
        ImGui::Spacing();

        return changed;
    }

    constinit static std::array VecColors = {
        GetAxisColor('X'),
        GetAxisColor('Y'),
        GetAxisColor('Z'),
        GetAxisColor('W')
    };

    constinit static std::array QuatColors = {
        GetAxisColor('X'),
        GetAxisColor('Y'),
        GetAxisColor('Z'),
        GetAxisColor('Q')
    };

    constinit static std::array VecLabels = {"X", "Y", "Z", "W"};

    inline bool EditVec2(
        const std::string& label, glm::vec2& value,
        const glm::vec2 reset = glm::zero<glm::vec2>(), const float speed = 0.1f,
        const float min = 0.0f, const float max = 0.0f
    ) {
        return EditVectorN<glm::vec2>(
            label,
            value,
            VecColors,
            VecLabels,
            reset,
            speed,
            min,
            max
        );
    }

    inline bool EditVec3(
        const std::string& label, glm::vec3& value,
        const glm::vec3 reset = glm::zero<glm::vec3>(), const float speed = 0.1f,
        const float min = 0.0f, const float max = 0.0f
    ) {
        return EditVectorN<glm::vec3>(
            label,
            value,
            VecColors,
            VecLabels,
            reset,
            speed,
            min,
            max
        );
    }

    inline bool EditVec4(
        const std::string& label, glm::vec4& value,
        const glm::vec4 reset = glm::zero<glm::vec4>(), const float speed = 0.1f,
        const float min = 0.0f, const float max = 0.0f
    ) {
        return EditVectorN<glm::vec4>(
            label,
            value,
            VecColors,
            VecLabels,
            reset,
            speed,
            min,
            max
        );
    }

    inline bool EditQuat(
        const std::string& label, glm::quat& quaternion,
        const glm::quat reset = glm::identity<glm::quat>(), const float speed = 0.1f,
        const float min = 0.0f, const float max = 0.0f
    ) {
        return EditVectorN<glm::quat>(
            label,
            quaternion,
            QuatColors,
            VecLabels,
            reset,
            speed,
            min,
            max
        );
    }

    inline bool EditColor4(const std::string& rawLabel, glm::vec4& value, const float resetValue = 1.0f) {
        const std::string label = BeautifyLabel(rawLabel);
        bool changed = false;

        ImGui::PushID(label.c_str());
        ImGui::Columns(2, nullptr, false);

        ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() / 3.0f);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(label.c_str());
        ImGui::NextColumn();

        const float totalWidth = ImGui::GetContentRegionAvail().x;

        const float lineHeight = GImGui->FontSize + GImGui->Style.FramePadding.y * 2.0f;
        const ImVec2 buttonSize = {lineHeight + 3.0f, lineHeight};

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.35f, 0.35f, 0.35f, 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.45f, 0.45f, 0.45f, 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.30f, 0.30f, 0.30f, 1));

        if (ImGui::Button("R", buttonSize)) {
            value = glm::vec4(resetValue);
            changed = true;
        }

        ImGui::PopStyleColor(3);
        ImGui::SameLine();

        const float colorWidth = totalWidth - buttonSize.x - ImGui::GetStyle().ItemSpacing.x;

        ImGui::PushItemWidth(colorWidth);

        changed |= ImGui::ColorEdit4(
            "##color",
            glm::value_ptr(value),
            ImGuiColorEditFlags_DisplayRGB |
            ImGuiColorEditFlags_AlphaBar |
            ImGuiColorEditFlags_Float |
            ImGuiColorEditFlags_HDR
        );

        ImGui::PopItemWidth();

        ImGui::Columns(1);
        ImGui::PopID();
        ImGui::Spacing();

        return changed;
    }

    template <typename TEnum>
        requires (std::is_enum_v<TEnum>)
    bool EditEnum(const std::string& label, TEnum& value) {
        bool changed = false;

        ImGui::PushID(label.c_str());

        ImGui::Columns(2, nullptr, false);

        ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() / 3.0f);
        ImGui::Text(BeautifyLabel(label).c_str());
        ImGui::NextColumn();

        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

        if (ImGui::BeginCombo("##editEnum", magic_enum::enum_name(value).data())) {
            for (TEnum v : magic_enum::enum_values<TEnum>()) {
                bool selected = v == value;
                if (ImGui::Selectable(magic_enum::enum_name(v).data(), selected)) {
                    value = v;
                    changed = true;
                }

                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }

            ImGui::EndCombo();
        }

        ImGui::PopItemWidth();

        ImGui::PopStyleVar();

        ImGui::Columns(1);

        ImGui::PopID();

        ImGui::Spacing();

        return changed;
    }
}

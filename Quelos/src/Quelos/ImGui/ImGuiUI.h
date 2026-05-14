#pragma once

#include "imgui.h"
#include "imgui_internal.h"

#include "icons_font_awesome.h"

#include "spdlog/fmt/fmt.h"
#include "magic_enum/magic_enum.hpp"

#include "Quelos/Math/Math.h"

#include "Quelos/Utility/TupleLike.h"
#include "Quelos/AssetManager/AssetManager.h"

namespace Quelos::UI {
    // Formats a string to a temporary buffer
    // There is no guarantee that the cstr will be valid after the next call
    // So either copy the result or use it immediately
    // But should be good for most ImGui calls since those copy the buffer internally
    template <typename... Args>
    const char* FormatTemp(fmt::format_string<Args...> fmtStr, Args&&... args) {
        // Ring buffer for extra safety
        // (I think? cases where you're formatting more than once in a single line for some reason)
        constexpr int k_BufferCount = 4;
        thread_local fmt::memory_buffer buffers[k_BufferCount];
        thread_local uint32_t index = 0;

        auto& buffer = buffers[index++ % k_BufferCount];
        buffer.clear();

        fmt::format_to(std::back_inserter(buffer), fmtStr, std::forward<Args>(args)...);

        buffer.push_back('\0');
        return buffer.data();
    }

    inline bool EditFloat(
        const std::string& label,
        float& value,
        const float speed = 0.1f,
        const float min = 0,
        const float max = 0
    ) {
        ImGui::PushID(label.c_str());

        ImGui::Columns(2, nullptr, false);

        ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() / 3.0f);
        ImGui::TextUnformatted(label.c_str());
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
        default: std::unreachable();
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
        const AxisColor& color
    ) {
        bool changed = false;

        ImGui::PushItemWidth(dragWidth);

        ImGui::SetNextItemColorMarker(ImColor(color.Base));
        changed |= ImGui::DragFloat(
            FormatTemp("##{}", label),
            &value,
            speed,
            min,
            max,
            "%.2f",
            ImGuiSliderFlags_ColorMarkers
        );
        ImGui::PopItemWidth();

        return changed;
    }

    template <FloatType T>
    bool EditFloatN(
        const std::string_view label,
        T& value,
        const std::array<AxisColor, 4> colors,
        const std::array<const char*, 4> axisLabels,
        const T& resetValue,
        const float speed,
        const float min,
        const float max
    ) {
        bool changed = false;
        const uint32_t count = math::count<T>();

        ImGui::PushID(FormatTemp("{}", label));
        ImGui::Columns(2, nullptr, false);

        ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() / 3.0f);
        ImGui::TextUnformatted(FormatTemp("{}", label));

        if (ImGui::BeginPopupContextItem("FieldContextMenu")) {
            if (ImGui::MenuItem(FormatTemp("{} {}", ICON_FA_ALIGN_RIGHT, "Reset"))) {
                value = resetValue;
                changed = true;
            }

            ImGui::EndPopup();
        }

        ImGui::NextColumn();

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {4, 0});

        const float totalWidth = ImGui::GetContentRegionAvail().x;
        const float spacing = ImGui::GetStyle().ItemSpacing.x;

        const float totalSpacingWidth = spacing * (count - 1);

        float dragWidth = (totalWidth - totalSpacingWidth) / count;

        ImGui::BeginGroup();

        for (int i = 0; i < count; i++) {
            if (i == count - 1) {
                dragWidth = ImGui::GetContentRegionAvail().x;
            }

            changed |= DrawAxis(
                axisLabels[i],
                value.f32[i],
                resetValue.f32[i],
                speed,
                min,
                max,
                dragWidth,
                colors[i]
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
        GetAxisColor('Q'),
        GetAxisColor('X'),
        GetAxisColor('Y'),
        GetAxisColor('Z')
    };

    constinit static std::array VecLabels = {"X", "Y", "Z", "W"};

    constinit static std::array QuatLabels = {"W", "X", "Y", "Z"};

    inline bool EditFloat2(
        const std::string& label, float2& value,
        const float2& reset = float2(0.0f), const float speed = 0.1f,
        const float min = 0.0f, const float max = 0.0f
    ) {
        return EditFloatN<float2>(
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

    inline bool EditFloat3(
        const std::string_view label, float3& value,
        const float3& reset = float3(0.0f), const float speed = 0.1f,
        const float min = 0.0f, const float max = 0.0f
    ) {
        return EditFloatN<float3>(
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

    inline bool EditFloat4(
        const std::string& label, float4& value,
        const float4& reset = float4(0.0f), const float speed = 0.1f,
        const float min = 0.0f, const float max = 0.0f
    ) {
        return EditFloatN<float4>(
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
        const std::string& label, quaternion& value,
        const quaternion& reset = quaternion::identity(), const float speed = 0.1f,
        const float min = 0.0f, const float max = 0.0f
    ) {
        return EditFloatN(
            label,
            value,
            QuatColors,
            QuatLabels,
            reset,
            speed,
            min,
            max
        );
    }

    inline bool EditColor4(const std::string& label, float4& value, const float resetValue = 1.0f) {
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
            value = float4(resetValue);
            changed = true;
        }

        ImGui::PopStyleColor(3);
        ImGui::SameLine();

        const float colorWidth = totalWidth - buttonSize.x - ImGui::GetStyle().ItemSpacing.x;

        ImGui::PushItemWidth(colorWidth);

        changed |= ImGui::ColorEdit4(
            "##edotColor4",
            &value.x,
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
        ImGui::TextUnformatted(label.c_str());
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

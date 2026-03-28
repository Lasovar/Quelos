#pragma once

#include "imgui.h"
#include "imgui_internal.h"

#include "icons_font_awesome.h"

#include "fmt/format.h"

#include "glm/vec3.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "Quelos/Utility/TupleLike.h"
#include "Quelos/AssetManager/AssetManager.h"

namespace Quelos::UI {
    template <typename... Args>
    const char* FormatTemp(fmt::format_string<Args...> fmtStr, Args&&... args) {
        // Ring buffer for extra safety
        // (I think? cases where you're formatting more than once in a single line for some reason)
        constexpr int k_BufferCount = 4;
        thread_local fmt::memory_buffer buffers[k_BufferCount];
        thread_local int index = 0;

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
            FormatTemp("##{}", label),
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
        const std::string& label,
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
        GetAxisColor('Q'),
        GetAxisColor('X'),
        GetAxisColor('Y'),
        GetAxisColor('Z')
    };

    constinit static std::array VecLabels = {"X", "Y", "Z", "W"};

    constinit static std::array QuatLabels = {"W", "X", "Y", "Z"};

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
            QuatLabels,
            reset,
            speed,
            min,
            max
        );
    }

    inline bool EditColor4(const std::string& label, glm::vec4& value, const float resetValue = 1.0f) {
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
            "##edotColor4",
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

    template <typename T>
        requires (std::is_base_of_v<Asset, T>)
    bool EditAsset(const std::string& label, Ref<T>& value) {
        bool changed = false;

        ImGui::PushID(label.c_str());

        // Layout
        ImGui::Columns(2, nullptr, false);
        ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() / 3.0f);

        ImGui::TextUnformatted(label.c_str());
        ImGui::NextColumn();

        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);

        // Frame style
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 4));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));

        ImGui::BeginGroup();

        // Asset display name
        static std::string typeName(magic_enum::enum_name(T::GetStaticType()));
        static std::string none = fmt::format("None ({})", typeName);

        const char* assetName = nullptr;

        if (value) {
            if (const AssetMetadata* meta = Project::GetEditorAssetManager()->
                GetAssetMetadata(value->GetAssetHandle())) {
                assetName = meta->FilePath.filename().c_str();
            }
        }

        constexpr float resetButtonSize = 30.0f;
        const ImVec2 size = {ImGui::GetContentRegionAvail().x - resetButtonSize * 2.0f, ImGui::GetFrameHeight()};

        ImGui::Dummy(size);

        ImDrawList* draw = ImGui::GetWindowDrawList();
        const ImVec2 min = ImGui::GetItemRectMin();
        const ImVec2 max = ImGui::GetItemRectMax();

        const ImU32 bg = ImGui::GetColorU32(ImGuiCol_FrameBg);
        const ImU32 border = ImGui::GetColorU32(ImGuiCol_Border);
        const ImU32 textCol = ImGui::GetColorU32(ImGuiCol_Text);

        // Draw frame
        draw->AddRectFilled(min, max, bg, 0.0f);
        draw->AddRect(min, max, border, 0.0f);

        // Text
        draw->AddText(
            ImVec2(min.x + 8.0f, min.y + 4.0f),
            textCol,
            assetName ? FormatTemp("{} ({})", assetName, typeName) : none.c_str()
        );

        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(typeName.c_str())) {
                if (const AssetHandle handle = *static_cast<const AssetHandle*>(payload->Data)) {
                    Ref<T> newAsset = AssetManager::GetAsset<T>(handle);

                    if (newAsset) {
                        value = newAsset;
                        changed = true;
                    }
                }
            }

            ImGui::EndDragDropTarget();
        }

        ImGui::SameLine();

        // Reset button
        if (ImGui::Button(ICON_FA_TRASH, ImVec2(resetButtonSize, 0.0f))) {
            value.reset();
            changed = true;
        }

        ImGui::SameLine();

        // Search button
        if (ImGui::Button("...", ImVec2(resetButtonSize, 0.0f))) {
            ImGui::OpenPopup("AssetSearchPopup");
        }

        // Search Popup
        if (ImGui::BeginPopup("AssetSearchPopup")) {
            ImGui::Text("Search not implemented yet");

            // TODO: implement a fuzzy asset search

            ImGui::EndPopup();
        }

        ImGui::EndGroup();

        ImGui::PopStyleVar(2);
        ImGui::PopItemWidth();

        ImGui::Columns(1);
        ImGui::PopID();

        ImGui::Spacing();

        return changed;
    }
}

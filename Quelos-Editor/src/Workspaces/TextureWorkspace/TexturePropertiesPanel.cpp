//
// Created by lasovar on 10/25/26.
//

#include "TexturePropertiesPanel.hpp"

#include "EditorUI.h"

namespace QuelosEditor {
    TexturePropertiesPanel::TexturePropertiesPanel(UndoSystem& undoSystem, const AssetRef<Texture>& texture)
        : m_Texture(texture), m_UndoSystem(undoSystem)
    {
        if (m_Texture) {
            const TextureType textureType = m_Texture.Get().GetType();
            for (const auto& type : magic_enum::enum_values<TextureType>()) {
                if (textureType == type) {
                    break;
                }

                m_SelectedType++;
            }
        }
    }

    void TexturePropertiesPanel::OnImGuiRender(ImGuiID dockspaceID, const ImGuiWindowClass& windowClass) {
        if (!m_Texture) {
            return;
        }

        if (UI::Begin("Properties", dockspaceID, windowClass)) {
            static constexpr auto textureTypes = magic_enum::enum_names<TextureType>();

            if (ImGui::BeginCombo("Texture Type", FormatTemp("{}", textureTypes[m_SelectedType]))) {
                for (uint32_t i = 0; i < textureTypes.size(); i++) {
                    if (ImGui::Selectable(FormatTemp("{}", textureTypes[i]), m_SelectedType == i)) {
                        m_SelectedType = i;
                    }
                }

                ImGui::EndCombo();
            }
        }
        UI::End();
    }
} // QuelosEditor

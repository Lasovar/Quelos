//
// Created by lasovar on 10/25/26.
//

#include "TexturePropertiesPanel.hpp"

#include "EditorUI.h"
#include "AssetManagement/SidecarHandler.hpp"

namespace QuelosEditor {
    TexturePropertiesPanel::TexturePropertiesPanel(
        UndoSystem& undoSystem,
        const AssetMetadata& assetMetadata,
        const AssetRef<Texture>& texture
    ) : m_Asset(assetMetadata), m_Texture(texture), m_UndoSystem(undoSystem)
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

        if (auto r = SidecarHandler::ReadSidecar(m_Asset.Type.GetName(), m_Asset.FilePath)) {
            r->ReadSettings(m_Settings);
        } else {
            QS_ERROR_TAGGED("Failed to read texture settings: {}", r.error());
        }
    }

    void TexturePropertiesPanel::OnImGuiRender(ImGuiID dockspaceID, const ImGuiWindowClass& windowClass) {
        if (!m_Texture) {
            return;
        }

        if (UI::Begin("Properties", dockspaceID, windowClass)) {
            static constexpr auto textureTypes = magic_enum::enum_names<TextureType>();

            if (UI::EditEnum("Usage", m_Settings.Usage)) {
                m_IsPendingChanges = true;
            }

            ImGui::Spacing();

            if (UI::EditEnum("Filter Mode", m_Settings.Filter)) {
                m_IsPendingChanges = true;
            }
            if (UI::EditEnum("Wrap Mode", m_Settings.Wrap)) {
                m_IsPendingChanges = true;
            }

            ImGui::Spacing();
            if (UI::EditEnum("Compression Quality", m_Settings.Compression)) {
                m_IsPendingChanges = true;
            }

            ImGui::Spacing();

            const bool disabled = !m_IsPendingChanges;
            if (disabled) {
                ImGui::BeginDisabled();
            }

            if (ImGui::Button("Save")) {
                if (auto r = SidecarHandler::WriteSettings(m_Asset.Type.GetName(), m_Asset.FilePath, m_Settings)) {
                    m_IsPendingChanges = false;
                } else {
                    QS_ERROR_TAGGED("Failed to write texture settings: {}", r.error());
                }
            }

            if (disabled) {
                ImGui::EndDisabled();
            }
        }
        UI::End();
    }
} // QuelosEditor

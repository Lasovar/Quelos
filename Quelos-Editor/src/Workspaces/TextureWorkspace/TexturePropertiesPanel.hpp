//
// Created by lasovar on 10/25/26.
//

#pragma once

#include "UndoSystem.h"
#include "AssetManagement/AssetImporters/TextureImporter.h"

namespace QuelosEditor {
    class TexturePropertiesPanel {
    public:
        TexturePropertiesPanel(UndoSystem& undoSystem, const AssetMetadata& assetMetadata, const AssetRef<Texture>& texture);

        void OnImGuiRender(ImGuiID dockspaceID, const ImGuiWindowClass& windowClass);
    private:
        uint32_t m_SelectedType = 0;
        const AssetMetadata& m_Asset;
        AssetRef<Texture> m_Texture;
        UndoSystem& m_UndoSystem;
        TextureSettings m_Settings;
        bool m_IsPendingChanges = false;
    };
} // QuelosEditor


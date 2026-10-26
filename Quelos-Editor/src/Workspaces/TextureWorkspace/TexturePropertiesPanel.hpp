//
// Created by lasovar on 10/25/26.
//

#pragma once
#include "UndoSystem.h"

namespace QuelosEditor {
    class TexturePropertiesPanel {
    public:
        TexturePropertiesPanel(UndoSystem& undoSystem, const AssetRef<Texture>& texture);

        void OnImGuiRender(ImGuiID dockspaceID, const ImGuiWindowClass& windowClass);
    private:
        uint32_t m_SelectedType = 0;
        AssetRef<Texture> m_Texture;
        UndoSystem& m_UndoSystem;
    };
} // QuelosEditor


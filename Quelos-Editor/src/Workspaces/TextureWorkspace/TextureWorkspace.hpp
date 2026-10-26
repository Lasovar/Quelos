//
// Created by lasovar on 10/25/26.
//

#pragma once
#include "TexturePropertiesPanel.hpp"
#include "Workspaces/Workspace.h"

namespace QuelosEditor {
    class TextureWorkspace : public Workspace {
    public:
        TextureWorkspace(UndoSystem& undoSystem, const AssetMetadata& metadata);

        void Tick(float deltaTime) override;
        void WorkspaceContents() override;
        void OnEvent(Event& event) override;
    private:
        AssetRef<Texture> m_Texture;
        TextureViewHandle m_TextureView;
        TexturePropertiesPanel m_PropertiesPanel;
    };
} // QuelosEditor

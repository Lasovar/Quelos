//
// Created by lasovar on 5/29/26.
//

#pragma once
#include "MaterialPropertiesPanel.h"
#include "Workspaces/Workspace.h"

namespace QuelosEditor {
    class MaterialWorkspace : public Workspace {
    public:
        MaterialWorkspace(UndoSystem& undoSystem, const AssetMetadata& metadata);

        void Tick(float deltaTime) override;
        void WorkspaceContents() override;
        void OnEvent(Event& event) override;

        ~MaterialWorkspace() override;

    private:
        AssetRef<Material> m_Material;
        MaterialPropertiesPanel m_PropertiesPanel;
    };
}

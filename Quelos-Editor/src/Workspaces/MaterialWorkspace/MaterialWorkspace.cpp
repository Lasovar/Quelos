//
// Created by lasovar on 5/29/26.
//

#include "MaterialWorkspace.h"

#include "AssetManagement/AssetImporters/MaterialImporter.h"
#include "Quelos/Utility/FileSystem.h"

namespace QuelosEditor {
    MaterialWorkspace::MaterialWorkspace(UndoSystem& undoSystem, const AssetMetadata& metadata)
        : Workspace(std::string(FS::Stem(metadata.FilePath)), undoSystem), m_Material(metadata.Handle),
          m_PropertiesPanel(undoSystem, m_Material)
    {
    }

    void MaterialWorkspace::Tick(float deltaTime) {}

    void MaterialWorkspace::WorkspaceContents() {
        m_PropertiesPanel.OnImGuiRender(m_WorkspaceID, m_WorkspaceClass);
    }

    void MaterialWorkspace::OnEvent(Event& event) {}

    MaterialWorkspace::~MaterialWorkspace() {
        MaterialImporter::SaveMaterial(m_Material);
    }
}

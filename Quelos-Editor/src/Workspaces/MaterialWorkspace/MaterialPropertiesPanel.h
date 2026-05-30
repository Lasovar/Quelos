//
// Created by lasovar on 5/29/26.
//

#pragma once
#include "UndoSystem.h"
#include "Quelos/AssetManager/AssetRef.h"
#include "Quelos/Renderer/Material.h"

namespace QuelosEditor {
    using namespace Quelos;

    class MaterialPropertiesPanel {
    public:
        MaterialPropertiesPanel(UndoSystem& undoSystem, const AssetRef<Material>& material)
            : m_Material(material), m_UndoSystem(undoSystem) {}

        void OnImGuiRender(ImGuiID dockspaceID, const ImGuiWindowClass& windowClass) const;

    private:
        AssetRef<Material> m_Material;
        UndoSystem& m_UndoSystem;
    };
}

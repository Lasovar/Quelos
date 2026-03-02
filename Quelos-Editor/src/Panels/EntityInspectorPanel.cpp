#include "EntityInspectorPanel.h"

#include "imgui.h"
#include "InspectorArchive.h"

namespace Quelos {
    void EntityInspectorPanel::OnImGuiRender(const ImGuiID dockspaceID, const ImGuiWindowClass& windowClass) {
        ImGui::SetNextWindowDockID(dockspaceID, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowClass(&windowClass);

        if (ImGui::Begin("Entity Inspector")) {
            if (m_SelectedEntity.IsAlive()) {
                UndoSystem undo;
                InspectorArchive archive(m_SelectedEntity, m_Scene, undo);

                auto transform = m_SelectedEntity.GetRef<TransformComponent>();
                TransformComponent::Serialize(archive, *transform.Get());
            }
        }
        ImGui::End();
    }
}

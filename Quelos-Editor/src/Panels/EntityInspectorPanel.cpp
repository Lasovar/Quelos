#include "EntityInspectorPanel.h"

#include "imgui.h"
#include "InspectorArchive.h"

namespace Quelos {
    void EntityInspectorPanel::OnImGuiRender(const ImGuiID dockspaceID, const ImGuiWindowClass& windowClass) {
        ImGui::SetNextWindowDockID(dockspaceID, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowClass(&windowClass);

        if (ImGui::Begin("Entity Inspector")) {
            if (m_SelectedEntity.IsAlive()) {
                static std::function<void(SetFieldArchive& archive, void* data)> serialize =
                    [](SetFieldArchive& archive, void* data) {
                        TransformComponent::Serialize(archive, *static_cast<TransformComponent*>(data));
                    };

                InspectorArchive archive(m_SelectedEntity, m_Scene->GetWorld().id<TransformComponent>(), m_Scene, m_UndoSystem, serialize);

                auto& transform = m_SelectedEntity.GetMut<TransformComponent>();
                TransformComponent::Serialize(archive, transform);
            }
        }
        ImGui::End();
    }
}

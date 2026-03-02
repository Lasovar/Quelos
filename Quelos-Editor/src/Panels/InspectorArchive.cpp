#include "InspectorArchive.h"

#include "Quelos/ImGui/ImGuiUI.h"

namespace Quelos {
    void InspectorArchive::DrawField(const std::string_view name, glm::vec3& value) {
        glm::vec3 temp = value;

        UI::EditVec3(std::string(name), temp, 0.0f, 0.0f); /* && ImGui::IsItemDeactivatedAfterEdit()*/
        /*m_UndoSystem.Push(
                    MakeSetFieldCommand<TransformComponent>(
                        m_Entity,
                        &value,
                        temp
                    )
                );*/
    }

    void InspectorArchive::DrawField(const std::string_view name, glm::quat& value) {
        glm::quat temp = value;

        if (UI::EditQuat(std::string(name), temp, 0.0f, 0.0f) && ImGui::IsItemDeactivatedAfterEdit()) {
            /*m_UndoSystem.Push(
                    MakeSetFieldCommand<TransformComponent>(
                        m_Entity,
                        &value,
                        temp
                    )
                );*/
        }
    }
}

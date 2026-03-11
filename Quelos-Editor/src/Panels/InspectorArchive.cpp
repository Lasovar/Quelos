#include "InspectorArchive.h"


namespace Quelos {
    void InspectorArchive::DrawField(std::string_view name, float& value) {
        static float startValue = 0.0f;
        static bool startedEditing = false;
        float temp = value;

        if (UI::EditFloat(std::string(name), temp)) {
            if (!startedEditing) {
                startedEditing = true;
                startValue = value;
            }

            value = temp;
        }

        if (ImGui::IsItemDeactivatedAfterEdit()) {
            m_UndoSystem.Push<SetField<float>>(
                m_Entity.GetUntypedRef(m_ComponentID),
                m_SerializeComponentFunc,
                name,
                startValue,
                value
            );

            startedEditing = false;
            QS_CORE_INFO("HELLO");
        }
    }

    void InspectorArchive::DrawField(std::string_view name, glm::vec3& value) {
        static auto startValue = glm::zero<glm::vec3>();
        static bool startedEditing = false;
        glm::vec3 temp = value;

        if (UI::EditVec3(std::string(name), temp)) {
            if (!startedEditing) {
                startedEditing = true;
                startValue = value;
            }

            value = temp;
        }

        if (ImGui::IsItemDeactivatedAfterEdit()) {
            m_UndoSystem.Push<SetField<glm::vec3>>(
                m_Entity.GetUntypedRef(m_ComponentID),
                m_SerializeComponentFunc,
                name,
                startValue,
                value
            );

            startedEditing = false;
        }
    }

    void InspectorArchive::DrawField(const std::string_view name, glm::quat& value) {
        static auto startValue = glm::identity<glm::quat>();
        static bool startedEditing = false;
        glm::quat temp = value;

        if (UI::EditQuat(std::string(name), temp)) {
            if (!startedEditing) {
                startedEditing = true;
                startValue = value;
            }

            value = temp;
        }

        if (ImGui::IsItemDeactivatedAfterEdit()) {
            m_UndoSystem.Push<SetField<glm::quat>>(
                m_Entity.GetUntypedRef(m_ComponentID),
                std::move(m_SerializeComponentFunc),
                name,
                startValue,
                value
            );

            startedEditing = false;
        }
    }
}

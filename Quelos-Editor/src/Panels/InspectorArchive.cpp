#include "qspch.h"
#include "InspectorArchive.h"

namespace QuelosEditor {
    void InspectorArchive::DrawField(std::string_view name, float& value) {
        static float startValue = 0.0f;
        static bool startedEditing = false;
        float temp = value;

        if (UI::EditFloat(GetFormattedFieldName(name), temp)) {
            if (!startedEditing) {
                startedEditing = true;
                startValue = value;
            }

            value = temp;
        }

        if (ImGui::IsItemDeactivatedAfterEdit()) {
            m_UndoSystem.Push<SetField<float>>(
                m_Scene->GetComponentRegistry().GetSerializableComponentInfo(m_ComponentID)->Guid,
                m_Actor.GetActorID(),
                m_Scene,
                m_SerializeComponentFunc,
                name,
                startValue,
                value
            );

            startedEditing = false;
        }
    }

    void InspectorArchive::DrawField(std::string_view name, float3& value) {
        static float3 startValue;
        static bool startedEditing = false;
        float3 temp = value;

        if (UI::EditFloat3(GetFormattedFieldName(name), temp)) {
            if (!startedEditing) {
                startedEditing = true;
                startValue = value;
            }

            value = temp;
        }

        if (ImGui::IsItemDeactivatedAfterEdit()) {
            m_UndoSystem.Push<SetField<float3>>(
                m_Scene->GetComponentRegistry().GetSerializableComponentInfo(m_ComponentID)->Guid,
                m_Actor.GetActorID(),
                m_Scene,
                m_SerializeComponentFunc,
                name,
                startValue,
                value
            );

            startedEditing = false;
        }
    }

    void InspectorArchive::DrawField(const std::string_view name, quaternion& value) {
        static quaternion startValue = quaternion::identity();
        static bool startedEditing = false;
        quaternion temp = value;

        if (UI::EditQuat(GetFormattedFieldName(name), temp)) {
            if (!startedEditing) {
                startedEditing = true;
                startValue = value;
            }

            value = temp;
        }

        if (ImGui::IsItemDeactivatedAfterEdit()) {
            m_UndoSystem.Push<SetField<quaternion>>(
                m_Scene->GetComponentRegistry().GetSerializableComponentInfo(m_ComponentID)->Guid,
                m_Actor.GetActorID(),
                m_Scene,
                m_SerializeComponentFunc,
                name,
                startValue,
                value
            );

            startedEditing = false;
        }
    }

    void InspectorArchive::DrawField(std::string_view name, Color& value) {
        static Color startValue;
        static bool startedEditing = false;
        Color temp = value;

        if (UI::EditColor4(GetFormattedFieldName(name), temp)) {
            if (!startedEditing) {
                startedEditing = true;
                startValue = value;
            }

            value = temp;
        }

        if (ImGui::IsItemDeactivatedAfterEdit()) {
            m_UndoSystem.Push<SetField<Color>>(
                m_Scene->GetComponentRegistry().GetSerializableComponentInfo(m_ComponentID)->Guid,
                m_Actor.GetActorID(),
                m_Scene,
                m_SerializeComponentFunc,
                name,
                startValue,
                value
            );

            startedEditing = false;
        }
    }
}

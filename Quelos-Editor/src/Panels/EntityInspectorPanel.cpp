#include "EntityInspectorPanel.h"

#include "imgui.h"
#include "InspectorArchive.h"

#include "Quelos/Scenes/ComponentRegistery.h"

namespace Quelos {
    template <typename TComponent>
        requires (IsSerializable<TComponent>)
    void RegisterComponent(
        flecs::world& world,
        HashMap<RuntimeID, InspectorComponent>& componentMap
    ) {
        static InspectorArchiveSerializeFn inspectorSerialize = [](InspectorArchive& archive, void* data) {
            TComponent::Serialize(archive, *static_cast<TComponent*>(data));
        };

        static SetFieldSerializeFn setFieldSerialize = [](SetFieldArchive& archive, void* data) {
            TComponent::Serialize(archive, *static_cast<TComponent*>(data));
        };

        componentMap[world.id<TComponent>()] = {
            inspectorSerialize, setFieldSerialize, std::string(ComponentRegistry::TypeName<TComponent>())
        };
    }

    template <typename... TComponent>
    void RegisterComponents(
        flecs::world& world,
        HashMap<RuntimeID, InspectorComponent>& componentMap
    ) {
        ([&] {
            RegisterComponent<TComponent>(world, componentMap);
        }(), ...);
    }

    template <typename... Component>
    void RegisterComponents(
        ComponentGroup<Component...>, flecs::world& world,
        HashMap<RuntimeID, InspectorComponent>& componentMap
    ) {
        RegisterComponents<Component...>(world, componentMap);
    }

    EntityInspectorPanel::EntityInspectorPanel(const Ref<Scene>& scene, UndoSystem& undoSystem)
        : m_Scene(scene), m_UndoSystem(undoSystem) {
        RegisterComponents(AllComponents{}, m_Scene->GetWorld(), m_InspectorArchiveSerialize);
    }

    void EntityInspectorPanel::OnImGuiRender(const ImGuiID dockspaceID, const ImGuiWindowClass& windowClass) {
        ImGui::SetNextWindowDockID(dockspaceID, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowClass(&windowClass);

        if (ImGui::Begin("Entity Inspector")) {
            if (m_SelectedEntity.IsAlive()) {
                ImGui::Text("Entity: %s", m_SelectedEntity.GetName());

                ImGui::Separator();

                m_SelectedEntity.GetID().each([this](const flecs::id runtimeId) {
                    const auto it = m_InspectorArchiveSerialize.find(runtimeId);
                    if (it == m_InspectorArchiveSerialize.end()) {
                        return;
                    }

                    const InspectorComponent& inspectorComponent = it->second;
                    if (ImGui::CollapsingHeader(inspectorComponent.ComponentName.c_str())) {
                        InspectorArchive archive(
                            m_SelectedEntity,
                            runtimeId,
                            m_Scene,
                            m_UndoSystem,
                            inspectorComponent.SetFieldSerializeFn
                        );

                        void* data = m_SelectedEntity.GetMut(runtimeId);
                        inspectorComponent.InspectorSerializeFn(archive, data);
                    }
                });
            }
        }
        ImGui::End();
    }
}

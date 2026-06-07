#include "EntityInspectorPanel.h"

#include "EditorLayer.h"
#include "imgui.h"
#include "InspectorArchive.h"

#include "Quelos/Scenes/ComponentRegistery.h"
#include "rapidfuzz/fuzz.hpp"
#include "Scene/Commands/SetEntityNameCommnad.h"
#include "EditorUI.h"

namespace QuelosEditor {
    HashMap<RuntimeID, InspectorComponent> EntityInspectorPanel::s_InspectorArchiveSerialize{};

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

        HashMap<Serialization::PathID, std::string> formattedFields;
        BeatifyFieldNamesArchive beatifyFieldNames{formattedFields};
        TComponent pseudoComponent;
        TComponent::Serialize(beatifyFieldNames, pseudoComponent);

        componentMap[world.id<TComponent>()] = {
            inspectorSerialize, setFieldSerialize, BeautifyLabel(TypeNameShort<TComponent>()), formattedFields
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

    EntityInspectorPanel::EntityInspectorPanel(const Ref<Scene>& scene, SceneWorkspace& sceneWorkspace, UndoSystem& undoSystem)
        : m_Scene(scene), m_SceneWorkspace(sceneWorkspace), m_UndoSystem(undoSystem)
    {
        m_SceneWorkspace.OnEntitySelectionChanged([this](const Entity entity) {
            SetInspectorEntityName(entity);
        });

        if (s_InspectorArchiveSerialize.empty()) {
            RegisterComponents(AllComponents{}, m_Scene->GetWorld(), s_InspectorArchiveSerialize);

            static auto transformInspector = [&](void* transformData, const InspectorComponent& component, const Entity entity) {
                LocalTransform& localTransform = *static_cast<LocalTransform*>(transformData);
                static float3 startValue(0.0f);
                static bool startedEditing = false;
                float3 temp = localTransform.Position;

                if (UI::EditFloat3("Position", temp)) {
                    if (!startedEditing) {
                        startedEditing = true;
                        startValue = localTransform.Position;
                    }

                    localTransform.Position = temp;
                }

                ComponentRegistry& registry = m_Scene->GetComponentRegistry();
                const flecs::world& world = m_Scene->GetWorld();

                if (ImGui::IsItemDeactivatedAfterEdit()) {
                    m_UndoSystem.Push<SetField<float3>>(
                        registry.GetSerializableComponentInfo(world.id<LocalTransform>())->Guid,
                        entity.Get<EntityID>(),
                        m_Scene,
                        component.SetFieldSerializeFn,
                        "position",
                        startValue,
                        localTransform.Position
                    );

                    startedEditing = false;
                }

                temp = math::degrees(math::euler(localTransform.Rotation));
                static float3 rotationValue;
                static bool startedEditingRotation = false;

                if (UI::EditFloat3("Rotation", startedEditingRotation ? rotationValue : temp)) {
                    if (!startedEditingRotation) {
                        startedEditingRotation = true;
                        startValue = math::euler(localTransform.Rotation);
                        rotationValue = temp;
                    }

                    localTransform.Rotation = math::normalize(quaternion::rotation_euler_zxy(math::radians(rotationValue)));
                }

                if (ImGui::IsItemDeactivatedAfterEdit()) {
                    m_UndoSystem.Push<SetField<quaternion>>(
                        registry.GetSerializableComponentInfo(world.id<LocalTransform>())->Guid,
                        entity.Get<EntityID>(),
                        m_Scene,
                        component.SetFieldSerializeFn,
                        "rotation",
                        math::normalize(quaternion::rotation_euler_zxy(startValue)),
                        localTransform.Rotation
                    );

                    startedEditingRotation = false;
                }

                temp = localTransform.Scale;

                if (UI::EditFloat3("Scale", temp)) {
                    if (!startedEditing) {
                        startedEditing = true;
                        startValue = localTransform.Scale;
                    }

                    localTransform.Scale = temp;
                }

                if (ImGui::IsItemDeactivatedAfterEdit()) {
                    m_UndoSystem.Push<SetField<float3>>(
                        registry.GetSerializableComponentInfo(world.id<LocalTransform>())->Guid,
                        entity.Get<EntityID>(),
                        m_Scene,
                        component.SetFieldSerializeFn,
                        "scale",
                        startValue,
                        localTransform.Scale
                    );

                    startedEditing = false;
                }
            };

            RegisterCustomInspector(CustomInspector {
                "Local Transform",
                ComponentRegistry::GetComponentID<LocalTransform>(),
                transformInspector
            });
        }
    }

    void EntityInspectorPanel::SetInspectorEntityName(const Entity& entity) {
        if (entity.IsValid()) {
            std::snprintf(m_EntityNameField.data(), m_EntityNameField.size(), "%s", entity.GetName());
        }
    }

    void EntityInspectorPanel::SetScene(const Ref<Scene>& scene) {
        m_Scene = scene;
    }

    static const char* s_AddComponentPopupName = "AddComponentPopup";

    void EntityInspectorPanel::ClearSelectedEntity() const {
        m_SceneWorkspace.SetSelectEntity({});
    }

    bool EntityInspectorPanel::ComponentHeader(const char* label, RuntimeID runtimeId) {
        const ImGuiStyle& style = ImGui::GetStyle();

        float height = ImGui::GetFrameHeight();
        float width = ImGui::GetContentRegionAvail().x - height - 3.0f;

        ImVec2 pos = ImGui::GetCursorScreenPos();

        ImGui::InvisibleButton(label, {width, height});
        const bool hovered = ImGui::IsItemHovered();
        const bool clicked = ImGui::IsItemClicked();

        auto& state = m_CollapsedComponents[m_SceneWorkspace.GetSelectedEntity()];

        if (clicked) {
            if (state.contains(runtimeId)) {
                state.erase(runtimeId);
            } else {
                state.emplace(runtimeId);
            }
        }

        const bool open = !state.contains(runtimeId);

        // background
        const ImU32 col = ImGui::GetColorU32(hovered ? ImGuiCol_HeaderHovered : ImGuiCol_Header);

        ImGui::GetWindowDrawList()->AddRectFilled(
            pos,
            {pos.x + width, pos.y + height},
            col,
            style.FrameRounding
        );

        // arrow
        ImGui::RenderArrow(
            ImGui::GetWindowDrawList(),
            {pos.x + 4, pos.y + 3},
            ImGui::GetColorU32(ImGuiCol_Text),
            open ? ImGuiDir_Down : ImGuiDir_Right,
            1.0f
        );

        // label
        ImGui::SetCursorScreenPos({pos.x + 24, pos.y + 2});
        ImGui::TextUnformatted(label);

        // menu button
        float buttonSize = height;
        ImGui::SetCursorScreenPos({pos.x + width + 3.0f, pos.y});

        if (ImGui::Button(ICON_FA_ELLIPSIS_V, {buttonSize, buttonSize})) {
            ImGui::OpenPopup("ComponentMenu");
        }

        if (ImGui::BeginPopup("ComponentMenu")) {
            if (ImGui::MenuItem("Remove Component")) {
                ComponentRegistry& registry = m_Scene->GetComponentRegistry();
                if (const auto* info = registry.GetSerializableComponentInfo(runtimeId)) {
                    m_UndoSystem.Push<RemoveComponentCommand>(
                        m_SceneWorkspace.GetSelectedEntity().Get<EntityID>(),
                        m_Scene,
                        info->Guid
                    );
                }
            }

            ImGui::EndPopup();
        }

        // move cursor below header
        ImGui::SetCursorScreenPos({pos.x, pos.y + height});

        ImGui::Spacing();

        return open;
    }

    void EntityInspectorPanel::OnImGuiRender(const ImGuiID dockspaceID, const ImGuiWindowClass& windowClass) {
        const Entity entity = m_SceneWorkspace.GetSelectedEntity();
        EntityID entityId;

        if (entity.IsValid()) {
            entityId = entity.Get<EntityID>();
        }

        if (UI::Begin("Entity Inspector", dockspaceID, windowClass)) {
            if (entity.IsAlive()) {
                ImGui::Spacing();

                ImGui::SameLine();

                ImGui::AlignTextToFramePadding();
                ImGui::Text(ICON_FA_CUBE);

                ImGui::SameLine();

                ImGui::InputTextEx(
                     "##entityName",
                     "Set the entity name",
                     m_EntityNameField.data(),
                     m_EntityNameField.size(),
                     {ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight()},
                     ImGuiInputTextFlags_None
                 );

                if (ImGui::IsItemDeactivatedAfterEdit()) {
                    m_UndoSystem.Push<SetEntityName>(
                        entityId,
                        m_Scene,
                        std::string(m_EntityNameField.data())
                    );

                    std::snprintf(
                        m_EntityNameField.data(), m_EntityNameField.size(),
                        "%s", entity.GetName()
                    );
                }

                ImGui::Separator();

                m_Scene->GetWorld().defer_begin();

                entity.GetInternalID().each([&](const flecs::id runtimeId) {
                    const auto it = s_InspectorArchiveSerialize.find(runtimeId);
                    if (it == s_InspectorArchiveSerialize.end()) {
                        return;
                    }

                    auto customInspector = m_CustomInspectors.find(runtimeId);
                    if (customInspector != m_CustomInspectors.end()) {
                        ImGui::PushID(runtimeId);
                        if (ComponentHeader(customInspector->second.ComponentName.c_str(), runtimeId)) {
                            customInspector->second.DrawFn(entity.GetMut(runtimeId), it->second, entity);
                        }
                        ImGui::PopID();

                        return;
                    }

                    ImGui::PushID(runtimeId);

                    const InspectorComponent& inspectorComponent = it->second;

                    if (ComponentHeader(inspectorComponent.ComponentName.c_str(), runtimeId)) {
                        InspectorArchive archive(
                            entity,
                            runtimeId,
                            m_Scene,
                            m_UndoSystem,
                            inspectorComponent.SetFieldSerializeFn,
                            inspectorComponent.FormattedFieldNames
                        );

                        void* data = entity.GetMut(runtimeId);
                        inspectorComponent.InspectorSerializeFn(archive, data);
                    }

                    ImGui::PopID();
                });

                ImGui::Spacing();

                if (ImGui::Button("Add Component", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
                    ImGui::OpenPopup(s_AddComponentPopupName);
                }

                if (ImGui::BeginPopup(s_AddComponentPopupName)) {
                    struct ComponentResult {
                        const SerializableComponentInfo* Info;
                        double Score;
                    };

                    static Vec<ComponentResult> results;

                    ComponentRegistry& componentRegistry = m_Scene->GetComponentRegistry();

                    if (ImGui::IsWindowAppearing()) {
                        ImGui::SetKeyboardFocusHere();
                    }

                    static uint32_t selectedIndex = 0;
                    static fmt::memory_buffer buffer;

                    buffer.clear();
                    if (ImGui::InputText(
                        FormatTemp("{}##componentSearch", ICON_FA_SEARCH),
                        buffer.data(),
                        buffer.capacity(),
                        ImGuiInputTextFlags_EnterReturnsTrue
                    )) {
                        if (!results.empty()) {
                            m_UndoSystem.Push<AddComponentCommand>(
                               entityId,
                               m_Scene,
                               results[selectedIndex].Info->Guid
                           );

                            ImGui::CloseCurrentPopup();
                        }
                    }

                    const std::string_view query = buffer.data();

                    results.clear();
                    for (auto& comp : componentRegistry.GetSerializableComponents()) {
                        std::string_view name = comp.Name;
                        const double nameScore = rapidfuzz::fuzz::WRatio(query, name);
                        const double pathScore = std::max({
                            rapidfuzz::fuzz::partial_ratio(query, comp.FullName),
                            rapidfuzz::fuzz::token_set_ratio(query, comp.FullName)
                        });

                        if (double score = 0.7f * nameScore + 0.3f * pathScore; score > 50.0f) {
                            results.push_back({&comp, score});
                        }
                    }

                    if (!results.empty()) {
                        std::ranges::sort(
                            results,
                            [](const ComponentResult& a, const ComponentResult& b) { return a.Score > b.Score; }
                        );

                        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
                            selectedIndex = std::min(
                                selectedIndex + 1,
                                static_cast<uint32_t>(results.size()) - 1
                            );
                        }

                        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
                            selectedIndex = std::max(selectedIndex - 1, 0u);
                        }

                        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                            ImGui::CloseCurrentPopup();
                        }

                        // Enter adds first result
                        if (ImGui::IsKeyPressed(ImGuiKey_Enter) &&
                            !results.empty() &&
                            !ImGui::IsAnyItemHovered())
                        {
                            m_UndoSystem.Push<AddComponentCommand>(
                                entityId,
                                m_Scene,
                                results[selectedIndex].Info->Guid
                            );

                            ImGui::CloseCurrentPopup();
                        }

                        for (uint32_t i = 0; i < results.size(); i++) {
                            const bool selected = selectedIndex == i;

                            if (ImGui::Selectable(
                                    FormatTemp(
                                        "{} ({})",
                                        results[i].Info->Name.c_str(), results[i].Info->FullName
                                    ),
                                    selected
                                )
                            ) {
                                m_UndoSystem.Push<AddComponentCommand>(
                                    entityId,
                                    m_Scene,
                                    results[i].Info->Guid
                                );

                                ImGui::CloseCurrentPopup();
                            }
                        }
                    }

                    ImGui::EndPopup();
                }


                m_Scene->GetWorld().defer_end();
            }
        } UI::End();
    }
}

#include "EntityInspectorPanel.h"

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

    EntityInspectorPanel::EntityInspectorPanel(const Ref<Scene>& scene, UndoSystem& undoSystem)
        : m_Scene(scene), m_UndoSystem(undoSystem) {
        if (s_InspectorArchiveSerialize.empty()) {
            RegisterComponents(AllComponents{}, m_Scene->GetWorld(), s_InspectorArchiveSerialize);
        }
    }

    static const char* s_AddComponentPopupName = "AddComponentPopup";

    bool EntityInspectorPanel::ComponentHeader(const char* label, RuntimeID runtimeId) {
        const ImGuiStyle& style = ImGui::GetStyle();

        float height = ImGui::GetFrameHeight();
        float width = ImGui::GetContentRegionAvail().x - height - 3.0f;

        ImVec2 pos = ImGui::GetCursorScreenPos();

        ImGui::InvisibleButton(label, {width, height});
        const bool hovered = ImGui::IsItemHovered();
        const bool clicked = ImGui::IsItemClicked();

        auto& state = m_ExpandedComponents[m_SelectedActor];

        if (clicked) {
            if (state.contains(runtimeId)) {
                state.erase(runtimeId);
            } else {
                state.emplace(runtimeId);
            }
        }

        const bool open = state.contains(runtimeId);

        // background
        const ImU32 col = ImGui::GetColorU32(hovered ? ImGuiCol_HeaderHovered : ImGuiCol_Header);

        ImGui::GetWindowDrawList()->AddRectFilled(
            pos,
            {pos.x + width, pos.y + height},
            col,
            style.FrameRounding
        );

        // arrow
        float arrowSize = height * 0.5f;
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
                        m_SelectedActor.GetActorID(),
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
        ImGui::PushID(m_SelectedActor.GetActorID());
        if (UI::Begin("Entity Inspector", dockspaceID, windowClass)) {
            if (m_SelectedActor.IsAlive()) {
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
                        m_SelectedActor.GetActorID(),
                        m_Scene,
                        std::string(m_EntityNameField.data())
                    );

                    std::snprintf(
                        m_EntityNameField.data(), m_EntityNameField.size(),
                        "%s", m_SelectedActor.GetName()
                    );
                }

                ImGui::Separator();

                m_Scene->GetWorld().defer_begin();

                m_SelectedActor.GetInternalID().each([&](const flecs::id runtimeId) {
                    const auto it = s_InspectorArchiveSerialize.find(runtimeId);
                    if (it == s_InspectorArchiveSerialize.end()) {
                        return;
                    }

                    ImGui::PushID(runtimeId);

                    const InspectorComponent& inspectorComponent = it->second;

                    if (ComponentHeader(inspectorComponent.ComponentName.c_str(), runtimeId)) {
                        InspectorArchive archive(
                            m_SelectedActor,
                            runtimeId,
                            m_Scene,
                            m_UndoSystem,
                            inspectorComponent.SetFieldSerializeFn,
                            inspectorComponent.FormattedFieldNames
                        );

                        void* data = m_SelectedActor.GetMut(runtimeId);
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
                    results.clear();

                    ComponentRegistry& componentRegistry = m_Scene->GetComponentRegistry();

                    static fmt::memory_buffer buffer;
                    buffer.clear();
                    ImGui::InputText(
                        UI::FormatTemp("{}##componentSearch", ICON_FA_SEARCH),
                        buffer.data(), buffer.capacity()
                    );

                    const std::string_view query = buffer.data();

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

                        for (auto& result : results) {
                            if (ImGui::Selectable(
                                UI::FormatTemp(
                                    "{} ({})",
                                    result.Info->Name.c_str(), result.Info->FullName)
                                )
                            ) {
                                m_UndoSystem.Push<AddComponentCommand>(
                                    m_SelectedActor.GetActorID(),
                                    m_Scene,
                                    result.Info->Guid
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

        ImGui::PopID();
    }
}

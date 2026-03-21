#include "qspch.h"
#include "EntityHierarchyPanel.h"

#include "imgui_internal.h"
#include "Quelos/Platform/bgfx/ImGui/icons_font_awesome.h"
#include "Scene/Commands/CreateActorCommand.h"
#include "Scene/Commands/SetParentCommand.h"

namespace Quelos {
    EntityHierarchyPanel::EntityHierarchyPanel(
        const Ref<Scene>& scene, UndoSystem& undoSystem
    ) : m_Scene(scene), m_UndoSystem(undoSystem)
    {
        m_EntitiesQuery = m_Scene->GetWorld().query_builder()
                                 .with<ActorTag>()
                                 .without(flecs::ChildOf, flecs::Wildcard)
                                 .build();
    }

    void EntityHierarchyPanel::OnImGuiRender(const ImGuiID dockspaceID, const ImGuiWindowClass& windowClass) {
        ImGui::SetNextWindowDockID(dockspaceID, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowClass(&windowClass);

        if (ImGui::Begin("Hierarchy")) {
            if (ImGui::BeginPopupContextWindow("HierarchyContext", ImGuiPopupFlags_NoOpenOverItems)) {
                if (ImGui::MenuItem("Create Empty Actor")) {
                    ActorID actorId = ActorID::Generate();
                    m_UndoSystem.Push<CreateActor>(actorId, m_Scene);
                    SetSelectedEntity(m_Scene->GetActor(actorId));
                }

                if (ImGui::MenuItem("Paste")) {
                    // clipboard logic
                }

                ImGui::EndPopup();
            }

            m_EntitiesStack.clear();

            const flecs::world& world = m_Scene->GetWorld();
            world.defer_begin();
            m_EntitiesQuery.each([&](const flecs::entity entity) {
                DrawEntity(entity, 0, m_EntitiesStack);
            });

            ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y));

            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ACTOR")) {
                    const ActorID droppedId = *static_cast<ActorID*>(payload->Data);

                    if (droppedId.IsValid()) {
                        m_UndoSystem.Push<SetParent>(droppedId, ActorID(), m_Scene);
                    }
                }

                ImGui::EndDragDropTarget();
            }


            world.defer_end();

            const ImVec2 mouse = ImGui::GetIO().MousePos;
            const ImVec2 cursor = ImGui::GetCursorScreenPos();

            if (mouse.y > cursor.y &&
                ImGui::IsWindowHovered() &&
                ImGui::IsMouseClicked(0) &&
                !ImGui::IsAnyItemHovered())
            {
                SetSelectedEntity({});
            }
        }
        ImGui::End();
    }

    static bool IsDescendant(const Entity parent, Entity candidate) {
        while (candidate.IsValid()) {
            if (candidate == parent) {
                return true;
            }

            candidate = candidate.GetParent();
        }

        return false;
    }

    void EntityHierarchyPanel::DrawEntity(const Entity entity, const int depth, std::vector<bool>& stack) {
        ImGui::PushID(entity.GetID());

        // Row
        const bool selected = ImGui::Selectable("##row", m_Selected == entity, ImGuiSelectableFlags_SpanAllColumns);

        const int count = entity.ChildrenCount();
        const bool hasChildren = count > 0;

        const ImVec2 min = ImGui::GetItemRectMin();
        const ImVec2 max = ImGui::GetItemRectMax();

        auto* draw = ImGui::GetWindowDrawList();

        constexpr float indent = 20.0f;
        constexpr float arrowSize = 12.0f;

        const float baseX = min.x;
        const float centerY = (min.y + max.y) * 0.5f;

        // Tree Lines
        for (int i = 0; i < depth; i++) {
            if (stack[i]) {
                float x = baseX + i * indent + 12.0f;

                draw->AddLine(
                    ImVec2(x, min.y),
                    ImVec2(x, max.y),
                    IM_COL32(90, 90, 90, 255)
                );
            }
        }

        // Connector
        if (depth > 0) {
            const float x = baseX + (depth - 1) * indent + 12.0f;
            const bool hasNext = stack.back();

            // vertical part
            draw->AddLine(
                ImVec2(x, min.y),
                ImVec2(x, hasNext ? max.y : centerY),
                IM_COL32(90, 90, 90, 255)
            );

            // horizontal part
            draw->AddLine(
                ImVec2(x, centerY),
                ImVec2(hasChildren ? x + 12.0f : x + 24.0f, centerY),
                IM_COL32(90, 90, 90, 255)
            );
        }

        // Arrow
        const bool open = std::ranges::contains(m_OpenEntities, entity);
        const float arrowX = baseX + depth * indent + 4.0f;

        const ImRect arrowRect(
            ImVec2(arrowX, min.y),
            ImVec2(arrowX + arrowSize, max.y)
        );

        const bool hovered = arrowRect.Contains(ImGui::GetIO().MousePos);
        const bool clicked = hovered && ImGui::IsMouseReleased(0);

        if (clicked && hasChildren) {
            if (open) {
                m_OpenEntities.erase(entity);
            }
            else {
                m_OpenEntities.insert(entity);
            }
        } else if (selected) {
            SetSelectedEntity(entity);
        }

        // draw arrow
        if (hasChildren) {
            const ImU32 color = hovered
                                    ? IM_COL32(255, 255, 255, 255)
                                    : IM_COL32(180, 180, 180, 255);

            ImGui::RenderArrow(
                draw,
                ImVec2(arrowX, centerY - ImGui::GetFontSize() * 0.35f),
                color, open ? ImGuiDir_Down : ImGuiDir_Right,
                0.8f
            );
        }

        // Icon + Name
        const float textX = baseX + depth * indent + 24.0f;
        const std::string label = std::format("{} {}", ICON_FA_CUBE, entity.GetName());

        draw->AddText(
            ImVec2(textX, centerY - ImGui::GetFontSize() * 0.5f),
            IM_COL32_WHITE,
            label.c_str()
        );

        if (ImGui::BeginPopupContextItem("EntityContext")) {
            m_Selected = entity;

            if (ImGui::MenuItem("Create Child")) {
                // your create logic
            }

            if (ImGui::MenuItem("Delete")) {
                m_UndoSystem.Push<DestroyActor>(entity.Get<ActorTag>().ID, m_Scene);
            }

            if (ImGui::MenuItem("Rename"))
            {
                // trigger rename state
            }

            ImGui::EndPopup();
        }

        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
            ActorID dragged = entity.Get<ActorTag>().ID;

            ImGui::SetDragDropPayload(
                "ACTOR",
                &dragged,
                sizeof(ActorID)
            );

            ImGui::Text("%s", entity.GetName());

            ImGui::EndDragDropSource();
        }

        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ACTOR")) {
                const ActorID droppedId = *static_cast<ActorID*>(payload->Data);

                if (droppedId.IsValid()) {
                    const Entity dropped = m_Scene->GetActor(droppedId);

                    if (dropped != entity) {
                        if (IsDescendant(dropped, entity)) {
                            const Entity parent = dropped.GetParent();

                            dropped.GetID().children([&](const flecs::entity child) {
                                if (parent.IsValid()) {
                                    m_UndoSystem.Push<SetParent>(
                                        child.get<ActorTag>().ID,
                                        parent.Get<ActorTag>().ID,
                                        m_Scene
                                    );
                                }
                                else {
                                    m_UndoSystem.Push<SetParent>(
                                        child.get<ActorTag>().ID,
                                        ActorID(),
                                        m_Scene
                                    );
                                }
                            });
                        }

                        m_UndoSystem.Push<SetParent>(
                            droppedId,
                            entity.Get<ActorTag>().ID,
                            m_Scene
                        );
                    }
                }
            }

            ImGui::EndDragDropTarget();
        }

        ImGui::PopID();

        // Children
        if (!open) {
            return;
        }

        int i = 0;
        entity.GetID().children([&](const flecs::entity child) {
            stack.push_back(i < count - 1);

            DrawEntity(child, depth + 1, stack);

            stack.pop_back();
            i++;
        });
    }
}

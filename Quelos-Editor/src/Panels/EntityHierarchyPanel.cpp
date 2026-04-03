#include "qspch.h"
#include "EntityHierarchyPanel.h"

#include "EditorUI.h"
#include "imgui_internal.h"

#include "Scene/Commands/CreateActorCommand.h"
#include "Scene/Commands/ReorderChildrenCommand.h"
#include "Scene/Commands/SetParentCommand.h"

#include "Quelos/ImGui/icons_font_awesome.h"

namespace QuelosEditor {
    EntityHierarchyPanel::EntityHierarchyPanel(
        const Ref<Scene>& scene, UndoSystem& undoSystem
    ) : m_Scene(scene), m_UndoSystem(undoSystem) {
        m_SceneRoot = m_Scene->GetSceneRoot().GetInternalID();
    }

    void EntityHierarchyPanel::OnImGuiRender(const ImGuiID dockspaceID, const ImGuiWindowClass& windowClass) {
        if (UI::Begin("Hierarchy", dockspaceID, windowClass)) {
            if (ImGui::BeginPopupContextWindow("HierarchyContext", ImGuiPopupFlags_NoOpenOverItems)) {
                if (ImGui::MenuItem("Create Empty Actor")) {
                    ActorID actorId = ActorID::Generate();
                    m_UndoSystem.Push<CreateActor>(actorId, m_Scene);
                    SetSelectedActor(m_Scene->GetActor(actorId));
                }

                if (ImGui::MenuItem("Paste")) {
                    // clipboard logic
                }

                ImGui::EndPopup();
            }

            const flecs::world& world = m_Scene->GetWorld();
            world.defer_begin();
            uint32_t order = 0;
            m_SceneRoot.GetInternalID().children([&](const flecs::entity entity) {
                m_EntitiesStack.clear();
                DrawActor(entity, 0, m_EntitiesStack, order);
                order++;
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

            if (m_RequestReorder) {
                m_UndoSystem.Push<ReorderChild>(
                    m_ReorderTarget,
                    m_ReorderTargetParent,
                    m_ReorderTargetAfter,
                    m_Scene
                );

                m_ReorderTargetParent = {};
                m_ReorderTargetAfter = {};
                m_ReorderTarget = {};

                m_RequestReorder = false;

                m_SceneRoot.IndexChildOrders();
            }

            const ImVec2 mouse = ImGui::GetIO().MousePos;
            const ImVec2 cursor = ImGui::GetCursorScreenPos();

            if (mouse.y > cursor.y &&
                ImGui::IsWindowHovered() &&
                ImGui::IsMouseClicked(0) &&
                !ImGui::IsAnyItemHovered()) {
                SetSelectedActor({});
            }
        } UI::End();
    }

    static bool IsDescendant(const Entity& parent, Entity candidate) {
        while (candidate.IsValid()) {
            if (candidate == parent) {
                return true;
            }

            candidate = candidate.GetParent();
        }

        return false;
    }

    void EntityHierarchyPanel::DrawActor(
        const Entity& actor,
        const int depth,
        std::vector<bool>& stack,
        const uint32_t order
    ) {
        ImGui::PushID(actor.GetInternalID());

        // Row
        const bool selected = ImGui::Selectable("##row", m_SelectedActor == actor);

        const int count = actor.ChildrenCount();
        const bool hasChildren = count > 0;

        const ImVec2 min = ImGui::GetItemRectMin();
        const ImVec2 max = ImGui::GetItemRectMax();

        auto* draw = ImGui::GetWindowDrawList();

        constexpr float indent = 20.0f;
        constexpr float arrowSize = 12.0f;

        const float baseX = min.x;
        const float centerY = (min.y + max.y) * 0.5f;

        constexpr uint32_t lineColor = IM_COL32(90, 90, 90, 255);

        // Tree Lines
        for (int i = 0; i < depth; i++) {
            if (stack[i]) {
                float x = baseX + i * indent + 12.0f;

                draw->AddLine(
                    ImVec2(x, min.y - 8),
                    ImVec2(x, max.y),
                    lineColor
                );
            }
        }

        // Connector
        if (depth > 0) {
            const float x = baseX + (depth - 1) * indent + 12.0f;
            const bool hasNext = stack.back();

            // vertical part
            draw->AddLine(
                ImVec2(x, min.y - 8),
                ImVec2(x, hasNext ? max.y : centerY),
                lineColor
            );

            // horizontal part
            draw->AddLine(
                ImVec2(x, centerY),
                ImVec2(hasChildren ? x + 12.0f : x + 24.0f, centerY),
                lineColor
            );
        }

        // Arrow
        const bool open = std::ranges::contains(m_OpenEntities, actor);
        const float arrowX = baseX + depth * indent + 4.0f;

        const ImRect arrowRect(
            ImVec2(arrowX, min.y),
            ImVec2(arrowX + arrowSize, max.y)
        );

        const bool hovered = arrowRect.Contains(ImGui::GetIO().MousePos);
        const bool clicked = hovered && ImGui::IsMouseReleased(0);

        if (open && !hasChildren) {
            m_OpenEntities.erase(actor);
        }

        if (clicked && hasChildren) {
            if (open) {
                m_OpenEntities.erase(actor);
            }
            else {
                m_OpenEntities.insert(actor);
            }
        }
        else if (selected) {
            SetSelectedActor(actor);
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
        const std::string label = std::format("{} {}", ICON_FA_CUBE, actor.GetName());

        draw->AddText(
            ImVec2(textX, centerY - ImGui::GetFontSize() * 0.5f),
            IM_COL32_WHITE,
            label.c_str()
        );

        if (ImGui::BeginPopupContextItem("EntityContext")) {
            m_SelectedActor = actor;

            if (ImGui::MenuItem("Create Child")) { }

            if (ImGui::MenuItem("Delete")) {
                m_UndoSystem.Push<DestroyActor>(actor.Get<ActorTag>().ID, m_Scene);
            }

            if (ImGui::MenuItem("Rename")) { }

            ImGui::EndPopup();
        }

        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
            ActorID dragged = actor.Get<ActorTag>().ID;

            ImGui::SetDragDropPayload(
                "ACTOR",
                &dragged,
                sizeof(ActorID)
            );

            ImGui::Text("%s", actor.GetName());

            ImGui::EndDragDropSource();
        }

        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ACTOR")) {
                const ActorID droppedId = *static_cast<ActorID*>(payload->Data);

                if (droppedId.IsValid()) {
                    const Actor dropped = m_Scene->GetActor(droppedId);

                    if (dropped != actor) {
                        if (IsDescendant(dropped, actor)) {
                            const Actor parent = dropped.GetParent();

                            dropped.GetInternalID().children([&](const flecs::entity child) {
                                if (parent.IsValid()) {
                                    m_UndoSystem.Push<SetParent>(
                                        child.get<ActorTag>().ID,
                                        parent.GetActorID(),
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
                            actor.Get<ActorTag>().ID,
                            m_Scene
                        );
                    }
                }
            }

            ImGui::EndDragDropTarget();
        }

        ImGui::PopID();

        // Children
        if (open) {
            const float separatorX = baseX + (depth + 1) * indent;
            const ImVec2 size(ImGui::GetContentRegionAvail().x, 3.0f);
            ImGui::SetCursorScreenPos({separatorX, ImGui::GetCursorScreenPos().y});
            ImGui::Dummy(size);

            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ACTOR")) {
                    const ActorID draggedId = *static_cast<ActorID*>(payload->Data);
                    const Actor dragged = m_Scene->GetActor(draggedId);

                    if (actor != dragged && !IsDescendant(dragged, actor)) {
                        m_ReorderTargetParent = actor.Get<ActorTag>().ID;
                        m_ReorderTarget = dragged.GetActorID();
                        m_ReorderTargetAfter = {};

                        m_RequestReorder = true;
                    }
                }

                ImGui::EndDragDropTarget();
            }

            int childOrder = 0;
            actor.GetInternalID().children([&](const flecs::entity child) {
                stack.push_back(childOrder < count - 1);

                DrawActor(child, depth + 1, stack, childOrder);

                stack.pop_back();
                childOrder++;
            });
        }

        const float separatorX = baseX + depth * indent;
        const ImVec2 size(ImGui::GetContentRegionAvail().x, 3.0f);
        ImGui::SetCursorScreenPos({separatorX, ImGui::GetCursorScreenPos().y - 3.0f});
        ImGui::Dummy(size);

        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ACTOR")) {
                const ActorID draggedId = *static_cast<ActorID*>(payload->Data);
                const Actor dragged = m_Scene->GetActor(draggedId);

                if (actor != dragged && !IsDescendant(dragged, actor)) {
                    const Actor aActor(actor);
                    const Actor parent = aActor.GetParent();

                    m_ReorderTargetParent = parent.GetActorID();
                    m_ReorderTarget = dragged.GetActorID();
                    m_ReorderTargetAfter = aActor.GetActorID();

                    m_RequestReorder = true;
                }
            }

            ImGui::EndDragDropTarget();
        }
    }
}

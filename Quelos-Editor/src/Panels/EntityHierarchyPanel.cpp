#include "qspch.h"
#include "EntityHierarchyPanel.h"

#include "EditorUI.h"
#include "imgui_internal.h"

#include "Scene/Commands/CreateActorCommand.h"
#include "Scene/Commands/ReorderChildrenCommand.h"
#include "Workspaces/SceneWorkspace.h"

#include "Quelos/ImGui/icons_font_awesome.h"

namespace QuelosEditor {
    static void GetDepthRange(const Vec<HierarchyRow>& rows, const int gapIndex, int& outMin, int& outMax) {
        outMax = gapIndex > 0 ? rows[gapIndex - 1].Depth + 1 : 0;
        outMin = gapIndex < static_cast<int>(rows.size()) ? rows[gapIndex].Depth : 0;
    }

    const HierarchyRow* FindRow(const Vec<HierarchyRow>& rows, const EntityID& id) {
        for (const auto& row : rows) {
            if (row.Id == id) {
                return &row;
            }
        }

        return nullptr;
    }

    EntityID FindLastChildId(const Entity& parent) {
        EntityID last;
        parent.GetInternalID().children([&](const flecs::entity child) {
            if (child.has<EntityID>()) {
                last = child.get<EntityID>();
            }
        });

        return last;
    }

    static int FindSubtreeEndIndex(const Vec<HierarchyRow>& rows, const int parentIndex) {
        const int parentDepth = rows[parentIndex].Depth;
        int end = parentIndex;

        for (int i = parentIndex + 1; i < static_cast<int>(rows.size()); ++i) {
            if (rows[i].Depth <= parentDepth) break;
            end = i;
        }

        return end;
    }

    static void ResolveParentAndSibling(
        const Vec<HierarchyRow>& rows,
        const int gapIndex,
        const int desiredDepth,
        EntityID& outParent,
        EntityID& outInsertAfter
    ) {
        for (int i = gapIndex - 1; i >= 0; --i) {
            if (rows[i].Depth == desiredDepth) {
                outParent = rows[i].Parent;
                outInsertAfter = rows[i].Id;
                return;
            }
            if (rows[i].Depth < desiredDepth) {
                // desiredDepth == rows[i].Depth + 1 here
                outParent = rows[i].Id;
                outInsertAfter = EntityID(); // front
                return;
            }
        }

        outParent = EntityID();
        outInsertAfter = EntityID();
    }

    static int FindHoveredRowIndex(const Vec<HierarchyRow>& rows, const float mouseY) {
        const auto it = std::ranges::upper_bound(rows, mouseY, {}, &HierarchyRow::Top);
        const int idx = static_cast<int>(std::distance(rows.begin(), it)) - 1;
        return idx >= 0 && mouseY < rows[idx].Bottom ? idx : -1;
    }

    float EntityHierarchyPanel::TrunkX(const float originX, const int depth) const {
        return originX + static_cast<float>(depth) * m_Indent + k_ConnectorXRatio * m_Indent;
    }

    float EntityHierarchyPanel::TextX(const float originX, const int depth) const {
        return originX + static_cast<float>(depth) * m_Indent + k_TextXRatio * m_Indent;
    }

    EntityHierarchyPanel::EntityHierarchyPanel(
        SceneWorkspace& sceneWorkspace, UndoSystem& undoSystem
    ) : m_SceneWorkspace(sceneWorkspace), m_UndoSystem(undoSystem) {}

    void EntityHierarchyPanel::SetScene(const Ref<Scene>& scene) {
        m_Scene = scene;
        m_SceneRoot = m_Scene->GetSceneRoot().GetInternalID();
    }

    void EntityHierarchyPanel::OnImGuiRender(const ImGuiID dockspaceID, const ImGuiWindowClass& windowClass) {
        if (UI::Begin("Hierarchy", dockspaceID, windowClass)) {
            if (ImGui::BeginPopupContextWindow("HierarchyContext", ImGuiPopupFlags_NoOpenOverItems)) {
                if (ImGui::MenuItem("Create Empty Actor")) {
                    EntityID actorId = EntityID::Generate();
                    m_UndoSystem.Push<CreateActor>(actorId, m_Scene);
                    const Actor actor = m_Scene->GetActor(actorId);
                    actor.OrderBack();
                    SetSelectedActor(actor);
                }

                if (ImGui::MenuItem("Paste")) {
                    // clipboard logic
                }

                ImGui::EndPopup();
            }

            const flecs::world& world = m_Scene->GetWorld();
            world.defer_begin();
            uint32_t order = 0;

            m_UIScale = ImGui::GetFontSize() / k_ReferenceFontSize;
            m_Indent = k_BaseIndent * m_UIScale;


            m_SelectedPath.clear();
            for (Entity cursor = m_SceneWorkspace.GetSelectedEntity(); cursor.IsValid(); cursor = cursor.GetParent()) {
                auto entityId = cursor.Get<EntityID>();
                if (!entityId) {
                    break;
                }

                m_SelectedPath.push_back(entityId);
            }
            std::ranges::reverse(m_SelectedPath);

            m_VisibleRows.clear();

            const ImVec2 listOrigin = ImGui::GetCursorScreenPos();
            const float availWidth = ImGui::GetContentRegionAvail().x;
            const float availHeight = ImGui::GetContentRegionAvail().y;

            m_SceneRoot.GetInternalID().children([&](const flecs::entity entity) {
                if (!entity.has<EntityID>()) {
                    return;
                }

                m_EntitiesStack.clear();
                DrawActor(entity, 0, m_EntitiesStack, EntityID());
                order++;
            });

            if (m_SelectedPath.size() > 1) {
                const ImU32 selColor = ImGui::GetColorU32(ImGui::GetStyleColorVec4(ImGuiCol_HeaderActive));
                DrawPathOverlay(listOrigin, m_SelectedPath, selColor, math::max(1.0f, 1.5f * m_UIScale));
            }

            const float drawnHeight = ImGui::GetCursorScreenPos().y - listOrigin.y;
            ResolveDragDrop(listOrigin, availWidth, math::max(drawnHeight, availHeight));

            world.defer_end();

            if (m_RequestReorder) {
                m_UndoSystem.Push<ReorderChild>(
                    m_ReorderTarget,
                    m_ReorderTargetParent,
                    m_ReorderTargetAfter,
                    m_Scene
                );

                if (m_ReorderTarget) {
                    SetSelectedActor(m_Scene->GetActor(m_ReorderTarget));
                }

                m_ReorderTargetParent = {};
                m_ReorderTargetAfter = {};
                m_ReorderTarget = {};

                m_RequestReorder = false;

                m_SceneRoot.IndexChildOrders();
            }

            const ImVec2 mouse = ImGui::GetIO().MousePos;
            const ImVec2 cursor = ImGui::GetCursorScreenPos();
            if (
                mouse.y > cursor.y &&
                ImGui::IsWindowHovered() &&
                ImGui::IsMouseClicked(0) &&
                !ImGui::IsAnyItemHovered()
            ) {
                SetSelectedActor({});
            }
        }
        UI::End();
    }

    void EntityHierarchyPanel::SetSelectedActor(const Actor& actor) const {
        m_SceneWorkspace.SetSelectEntity(static_cast<Entity>(actor));
    }

    void EntityHierarchyPanel::DrawActor(const Entity& actor, int depth, Vec<bool>& stack, const EntityID& parent) {
        ImGui::PushID(actor.GetInternalID());

        const ImVec2 actorCursorPosition = ImGui::GetCursorScreenPos();
        const bool selected = ImGui::Selectable("##row", m_SceneWorkspace.GetSelectedEntity() == actor);

        uint32_t count = 0;
        actor.GetInternalID().children([&count](const flecs::entity child) {
            if (child.has<EntityID>()) {
                count++;
            }
        });

        const bool hasChildren = count > 0;

        const ImVec2 min = ImGui::GetItemRectMin();
        const ImVec2 max = ImGui::GetItemRectMax();
        const EntityID actorId = actor.Get<EntityID>();

        m_VisibleRows.emplace_back(actorId, parent, depth, min.y, max.y, hasChildren);

        ImDrawList* draw = ImGui::GetWindowDrawList();
        const float baseX = min.x;
        const float centerY = (min.y + max.y) * 0.5f;
        const ImU32 lineColor = ImGui::GetColorU32(ImGui::GetStyleColorVec4(ImGuiCol_TreeLines));

        for (int i = 0; i < depth - 1; i++) {
            if (stack[i]) {
                const float x = TrunkX(baseX, i);
                draw->AddLine(ImVec2(x, min.y), ImVec2(x, max.y), lineColor, ImGui::GetStyle().TreeLinesSize);
            }
        }

        if (depth > 0) {
            const float x = TrunkX(baseX, depth - 1);
            const bool hasNext = stack.back();

            draw->AddLine(ImVec2(x, min.y), ImVec2(x, hasNext ? max.y : centerY), lineColor, ImGui::GetStyle().TreeLinesSize);
            draw->AddLine(ImVec2(x, centerY), ImVec2(hasChildren ? x + k_ConnectorXRatio * m_Indent : x + k_TextXRatio * m_Indent, centerY), lineColor, ImGui::GetStyle().TreeLinesSize);

            if (!hasNext) {
                //draw->AddCircleFilled(ImVec2(x, centerY), 1.5f, lineColor);
            }
        }

        const bool open = m_OpenEntities.contains(actor);
        const float arrowSize = m_Indent * k_ArrowRatio;
        const float arrowX = baseX + depth * m_Indent + m_Indent * k_ArrowInsetRatio;
        const ImRect arrowRect(ImVec2(arrowX, min.y), ImVec2(arrowX + arrowSize, max.y));
        const bool hovered = arrowRect.Contains(ImGui::GetIO().MousePos);
        const bool clicked = hovered && ImGui::IsMouseReleased(0);
        if (hasChildren && open) {
            const float x = TrunkX(baseX, depth);
            draw->AddLine(
                ImVec2(x, max.y),
                ImVec2(x, max.y - k_ArrowBaseConnector * m_UIScale),
                lineColor,
                ImGui::GetStyle().TreeLinesSize
            );
        }

        if (open && !hasChildren) {
            m_OpenEntities.erase(actor);
        }

        if (clicked && hasChildren) {
            if (open) m_OpenEntities.erase(actor);
            else m_OpenEntities.insert(actor);
        }
        else if (selected) {
            SetSelectedActor(actor);
        }

        if (hasChildren) {
            const ImU32 color = hovered ? IM_COL32(255, 255, 255, 255) : IM_COL32(180, 180, 180, 255);
            const auto& dc = ImGui::GetCurrentWindow()->DC;

            const char* label = open ? ICON_FA_CHEVRON_DOWN : ICON_FA_CHEVRON_RIGHT;
            const ImVec2 labelSize = ImGui::CalcTextSize(label, nullptr, true);
            const ImVec2 pos(arrowX, actorCursorPosition.y + dc.CurrLineTextBaseOffset);
            const ImRect bb(min, max);

            ImGui::PushStyleColor(ImGuiCol_Text, color);
            ImGui::RenderTextClipped(
                pos,
                ImVec2(max.x, pos.y + labelSize.y),
                label,
                nullptr,
                &labelSize,
                ImGui::GetStyle().SelectableTextAlign,
                &bb
            );
            ImGui::PopStyleColor();
        }

        const float textX = TextX(baseX, depth);
        const auto& dc = ImGui::GetCurrentWindow()->DC;

        const char* label = FormatTemp("{} {}", ICON_FA_CUBE, actor.GetName());
        const ImVec2 labelSize = ImGui::CalcTextSize(label, nullptr, true);
        const ImVec2 pos(textX, actorCursorPosition.y + dc.CurrLineTextBaseOffset);
        const ImRect bb(min, max);

        ImGui::RenderTextClipped(
            pos,
            ImVec2(max.x, pos.y + labelSize.y),
            label,
            nullptr,
            &labelSize,
            ImGui::GetStyle().SelectableTextAlign,
            &bb
        );

        if (ImGui::BeginPopupContextItem("EntityContext")) {
            SetSelectedActor(actor);
            if (ImGui::MenuItem("Create Child")) {
                EntityID childId = EntityID::Generate();
                m_UndoSystem.Push<CreateActor>(childId, actorId, m_Scene);
                const Actor child = m_Scene->GetActor(childId);
                child.OrderBack();
                SetSelectedActor(child);
            }
            if (ImGui::MenuItem("Delete")) {
                m_UndoSystem.Push<DestroyActor>(actorId, m_Scene);
            }
            if (ImGui::MenuItem("Rename")) {}
            ImGui::EndPopup();
        }

        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
            ImGui::SetDragDropPayload("ACTOR", &actorId, sizeof(EntityID));
            ImGui::Text("%s", actor.GetName());
            ImGui::EndDragDropSource();
        }

        ImGui::PopID();

        if (open) {
            int childOrder = 0;
            actor.GetInternalID().children([&](const flecs::entity child) {
                if (!child.has<EntityID>()) return;
                stack.push_back(childOrder < static_cast<int>(count) - 1);
                DrawActor(child, depth + 1, stack, actorId);
                stack.pop_back();
                childOrder++;
            });
        }
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

    void EntityHierarchyPanel::ResolveDragDrop(
        const ImVec2& listOrigin, const float listWidth, const float listHeight
    ) {
        ImGui::SetCursorScreenPos(listOrigin);
        ImGui::Dummy(ImVec2(listWidth, listHeight));

        if (!ImGui::BeginDragDropTarget()) {
            return;
        }

        constexpr ImGuiDragDropFlags flags =
            ImGuiDragDropFlags_AcceptBeforeDelivery |
            ImGuiDragDropFlags_AcceptNoDrawDefaultRect;

        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ACTOR", flags)) {
            const EntityID draggedId = *static_cast<const EntityID*>(payload->Data);
            const Actor dragged = m_Scene->GetActor(draggedId);
            const ImVec2 mouse = ImGui::GetMousePos();

            EntityID parent, insertAfter;
            int depth = 0;
            float lineY = 0.0f;

            const int hoveredIdx = FindHoveredRowIndex(m_VisibleRows, mouse.y);
            if (hoveredIdx < 0) {
                const int gapIndex = m_VisibleRows.empty() || mouse.y < m_VisibleRows.front().Top
                                         ? 0
                                         : static_cast<int>(m_VisibleRows.size());

                int minDepth, maxDepth;
                GetDepthRange(m_VisibleRows, gapIndex, minDepth, maxDepth);
                const int rawDepth = static_cast<int>(math::lround((mouse.x - listOrigin.x) / m_Indent));
                depth = math::clamp(rawDepth, minDepth, maxDepth);
                ResolveParentAndSibling(m_VisibleRows, gapIndex, depth, parent, insertAfter);
                lineY = GapY(gapIndex, listOrigin);
            }
            else {
                const HierarchyRow& row = m_VisibleRows[hoveredIdx];
                const float t = (mouse.y - row.Top) / math::max(1.0f, row.Bottom - row.Top);

                if (t < k_EdgeZoneRatio) {
                    depth = row.Depth;
                    ResolveParentAndSibling(m_VisibleRows, hoveredIdx, depth, parent, insertAfter);
                    lineY = row.Top;
                }
                else if (t > 1.0f - k_EdgeZoneRatio) {
                    const int gapIndex = hoveredIdx + 1;
                    int minDepth, maxDepth;
                    GetDepthRange(m_VisibleRows, gapIndex, minDepth, maxDepth);
                    const int rawDepth = static_cast<int>(math::lround((mouse.x - listOrigin.x) / m_Indent));
                    depth = math::clamp(rawDepth, minDepth, maxDepth);
                    ResolveParentAndSibling(m_VisibleRows, gapIndex, depth, parent, insertAfter);
                    lineY = row.Bottom;
                }
                else {
                    // reparent as last child
                    parent = row.Id;
                    depth = row.Depth + 1;

                    const int endIdx = FindSubtreeEndIndex(m_VisibleRows, hoveredIdx);
                    lineY = m_VisibleRows[endIdx].Bottom;

                    if (const Actor parentActor = m_Scene->GetActor(parent); parentActor.IsValid()) {
                        insertAfter = FindLastChildId(static_cast<Entity>(parentActor));
                    }
                }
            }

            bool valid = parent != draggedId && insertAfter != draggedId;
            if (valid && parent.IsValid()) {
                if (const Actor parentActor = m_Scene->GetActor(parent);
                    parentActor.IsValid() && IsDescendant(dragged, static_cast<Entity>(parentActor))) {
                    valid = false;
                }
            }

            if (valid) {
                if (parent.IsValid()) {
                    if (const HierarchyRow* parentRow = FindRow(m_VisibleRows, parent)) {
                        //DrawParentHighlight(listOrigin, listWidth, *parentRow);

                        const ImU32 dragColor = ImGui::GetColorU32(ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));
                        DrawHopConnector(listOrigin, listWidth, *parentRow, lineY, depth, dragColor, 2.0f);
                    }
                }

                //DrawInsertionIndicator(listOrigin, lineY, depth);

                if (payload->Delivery) {
                    m_ReorderTargetParent = parent;
                    m_ReorderTarget = draggedId;
                    m_ReorderTargetAfter = insertAfter;
                    m_RequestReorder = true;

                    if (parent.IsValid()) {
                        if (const Actor parentActor = m_Scene->GetActor(parent); parentActor.IsValid()) {
                            m_OpenEntities.insert(parentActor);
                        }
                    }
                }
            }
        }

        ImGui::EndDragDropTarget();
    }

    float EntityHierarchyPanel::GapY(const int gapIndex, const ImVec2& listOrigin) const {
        if (m_VisibleRows.empty()) {
            return listOrigin.y;
        }

        if (gapIndex <= 0) {
            return m_VisibleRows.front().Top;
        }

        if (gapIndex >= static_cast<int>(m_VisibleRows.size())) {
            return m_VisibleRows.back().Bottom;
        }

        return m_VisibleRows[gapIndex].Top;
    }

    void EntityHierarchyPanel::DrawInsertionIndicator(const ImVec2& listOrigin, const float y, const int depth) const {
        const float x0 = TextX(listOrigin.x, depth);

        ImDrawList* draw = ImGui::GetWindowDrawList();
        const ImU32 color = ImGui::GetColorU32(ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));
        draw->AddCircleFilled(ImVec2(x0, y), 2.5f, color);
    }

    void EntityHierarchyPanel::DrawHopConnector(
        const ImVec2& listOrigin, const float listWidth, const HierarchyRow& parentRow, const float childY, const int childDepth,
        const ImU32 color, const float thickness
    ) const {
        const float trunkX = TrunkX(listOrigin.x, parentRow.Depth) - ImGui::GetStyle().ItemInnerSpacing.x;
        const float parentY = parentRow.Bottom - k_ArrowBaseConnector * m_UIScale;

        ImDrawList* draw = ImGui::GetWindowDrawList();
        draw->AddLine(ImVec2(trunkX, parentY), ImVec2(trunkX, childY), color, thickness);
        draw->AddLine(
            ImVec2(trunkX, childY),
            ImVec2(listWidth == 0.0f ? TextX(listOrigin.x, childDepth) : listOrigin.x + listWidth, childY),
            color,
            thickness
        );
    }

    void EntityHierarchyPanel::DrawPathOverlay(
        const ImVec2& listOrigin, const Vec<EntityID>& chainRootFirst, const ImU32 color, const float thickness
    ) const {
        for (size_t i = 0; i + 1 < chainRootFirst.size(); ++i) {
            const HierarchyRow* parentRow = FindRow(m_VisibleRows, chainRootFirst[i]);
            const HierarchyRow* childRow = FindRow(m_VisibleRows, chainRootFirst[i + 1]);
            if (!parentRow || !childRow) {
                continue; // ancestor currently collapsed/not visible, skip that hop
            }

            const float childY = (childRow->Top + childRow->Bottom) * 0.5f;
            float width = childRow->HasChildren ? k_ConnectorXRatio * m_Indent : k_TextXRatio * m_Indent;
            const float trunkX = TrunkX(0, childRow->Depth - 1) - ImGui::GetStyle().ItemInnerSpacing.x;
            width += trunkX;

            DrawHopConnector(listOrigin, width, *parentRow, childY, childRow->Depth, color, thickness);
        }
    }
}

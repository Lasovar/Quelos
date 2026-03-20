#pragma once

#include "UndoSystem.h"

#include "imgui.h"

namespace Quelos {
    class Scene;

    using SelectionCallback = std::function<void(Entity)>;

    class EntityHierarchyPanel {
    public:
        EntityHierarchyPanel(const Ref<Scene>& scene, UndoSystem& undoSystem);

        void AddListenerOnEntitySelected(std::move_only_function<void(Entity)> callback) {
            m_OnSelectionChangedCallbacks.push_back(std::move(callback));
        };

        void OnImGuiRender(ImGuiID dockspaceID, const ImGuiWindowClass& windowClass);

    private:
        void SetSelectedEntity(const Entity entity) {
            m_Selected = entity;
            NotifyOnEntitySelectedListeners();
        }

        void NotifyOnEntitySelectedListeners() {
            for (auto& callback : m_OnSelectionChangedCallbacks) {
                callback(m_Selected);
            }
        }
        void DrawEntity(Entity entity, int depth, std::vector<bool>& stack);

    private:
        Ref<Scene> m_Scene;
        UndoSystem& m_UndoSystem;
        Vec<std::move_only_function<void(Entity)>> m_OnSelectionChangedCallbacks;

        flecs::query<> m_EntitiesQuery;

        // False for every last child
        Vec<bool> m_EntitiesStack;
        HashSet<Entity> m_OpenEntities;

        Entity m_Selected;
    };
}

#pragma once

#include "UndoSystem.h"

#include "imgui.h"

namespace Quelos {
    class Scene;

    using SelectionCallback = std::move_only_function<void(const Actor&)>;

    class EntityHierarchyPanel {
    public:
        EntityHierarchyPanel(const Ref<Scene>& scene, UndoSystem& undoSystem);

        void AddListenerOnEntitySelected(SelectionCallback callback) {
            m_OnSelectionChangedCallbacks.push_back(std::move(callback));
        };

        void OnImGuiRender(ImGuiID dockspaceID, const ImGuiWindowClass& windowClass);

    private:
        void SetSelectedActor(const Actor& actor) {
            m_SelectedActor = actor;
            NotifyOnEntitySelectedListeners();
        }

        void NotifyOnEntitySelectedListeners() {
            for (auto& callback : m_OnSelectionChangedCallbacks) {
                callback(m_SelectedActor);
            }
        }
        void DrawActor(const Entity& actor, int depth, std::vector<bool>& stack, uint32_t order);

    private:
        Ref<Scene> m_Scene;
        UndoSystem& m_UndoSystem;
        Vec<SelectionCallback> m_OnSelectionChangedCallbacks;

        Actor m_SceneRoot;

        // False for every last child
        Vec<bool> m_EntitiesStack;
        HashSet<Entity> m_OpenEntities;

        bool m_RequestReorder = false;
        ActorID m_ReorderTargetParent;
        ActorID m_ReorderTargetAfter;
        ActorID m_ReorderTarget;

        Actor m_SelectedActor;
    };
}

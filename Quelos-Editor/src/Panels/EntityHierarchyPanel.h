#pragma once

#include "UndoSystem.h"

#include "imgui.h"

namespace Quelos {
    class Scene;
}

namespace QuelosEditor {
    class SceneWorkspace;
    using namespace Quelos;

    class EntityHierarchyPanel {
    public:
        EntityHierarchyPanel(const Ref<Scene>& scene, SceneWorkspace& sceneWorkspace, UndoSystem& undoSystem);

        void OnImGuiRender(ImGuiID dockspaceID, const ImGuiWindowClass& windowClass);

    private:
        void SetSelectedActor(const Actor& actor) const;

        void DrawActor(const Entity& actor, int depth, std::vector<bool>& stack, uint32_t order);

    private:
        Ref<Scene> m_Scene;
        SceneWorkspace& m_SceneWorkspace;
        UndoSystem& m_UndoSystem;

        Actor m_SceneRoot;

        // False for every last child
        Vec<bool> m_EntitiesStack;
        HashSet<Entity> m_OpenEntities;

        bool m_RequestReorder = false;
        EntityID m_ReorderTargetParent;
        EntityID m_ReorderTargetAfter;
        EntityID m_ReorderTarget;
    };
}

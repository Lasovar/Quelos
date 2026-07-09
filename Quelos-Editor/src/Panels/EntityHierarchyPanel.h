#pragma once

#include "UndoSystem.h"

#include "imgui.h"

namespace Quelos {
    class Scene;
}

namespace QuelosEditor {
    class SceneWorkspace;
    using namespace Quelos;

    struct HierarchyRow {
        EntityID Id;
        EntityID Parent;
        int Depth;
        float Top;
        float Bottom;
        bool HasChildren;
    };

    class EntityHierarchyPanel {
    public:
        EntityHierarchyPanel(SceneWorkspace& sceneWorkspace, UndoSystem& undoSystem);

        void SetScene(const Ref<Scene>& scene);

        void OnImGuiRender(ImGuiID dockspaceID, const ImGuiWindowClass& windowClass);
    private:
        void SetSelectedActor(const Actor& actor) const;

        void DrawActor(const Entity& actor, int depth, Vec<bool>& stack, const EntityID& parent);
        void ResolveDragDrop(const ImVec2& listOrigin, float listWidth, float listHeight);
        void DrawInsertionIndicator(const ImVec2& listOrigin, float y, int depth) const;

        void DrawHopConnector(
            const ImVec2& listOrigin, float listWidth, const HierarchyRow& parentRow, float childY, int childDepth,
            ImU32 color, float thickness
        ) const;
        void DrawPathOverlay(
            const ImVec2& listOrigin, const Vec<EntityID>& chainRootFirst, ImU32 color, float thickness
        ) const;

        // Helpers
        float GapY(int gapIndex, const ImVec2& listOrigin) const;
        float TrunkX(float originX, int depth) const;
        float TextX(float originX, int depth) const;

    private:
        Ref<Scene> m_Scene;
        SceneWorkspace& m_SceneWorkspace;
        UndoSystem& m_UndoSystem;

        Actor m_SceneRoot;

        // False for every last child
        Vec<bool> m_EntitiesStack;
        Vec<HierarchyRow> m_VisibleRows;
        HashSet<Entity> m_OpenEntities;
        Vec<EntityID> m_SelectedPath;

        static constexpr float k_ReferenceFontSize = 17.0f;
        static constexpr float k_BaseIndent = 20.0f;
        static constexpr float k_ArrowRatio = 0.6f;
        static constexpr float k_ArrowBaseConnector = 3.0f;
        static constexpr float k_ConnectorXRatio = 0.55f;
        static constexpr float k_LeafRunRatio = 1.2f;
        static constexpr float k_ArrowInsetRatio = 0.2f;
        static constexpr float k_TextXRatio = 1.2f;
        static constexpr float k_EdgeZoneRatio = 0.25f; // top/bottom 25% each, middle 50%

        float m_UIScale = 1.0f;
        float m_Indent  = k_BaseIndent;

        bool m_RequestReorder = false;
        EntityID m_ReorderTargetParent;
        EntityID m_ReorderTargetAfter;
        EntityID m_ReorderTarget;
    };
}

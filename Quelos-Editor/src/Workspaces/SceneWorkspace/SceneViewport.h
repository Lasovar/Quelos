//
// Created by lasovar on 6/6/26.
//

#pragma once

#include "Quelos/Utility/Request.h"
#include "Panels/ViewportPanel.h"

#include "Quelos/ImGui/widgets/gizmo.h"

namespace QuelosEditor {
    class SceneWorkspace;

    class SceneViewport : public ViewportPanel {
    public:
        SceneViewport(
            std::string name,
            SceneWorkspace& sceneWorkspace,
            WorldRendererView&& worldRendererView,
            uint32_t width,
            uint32_t height
        );

        [[nodiscard]] const uint2& GetSelectRequestPosition() const { return m_SelectRequestPosition; }
        [[nodiscard]] Request& SelectRequest() { return m_SelectRequest; }

        void SetFrame(const Entity value, const float4x4& view, const float4x4& proj) {
            m_Selected = value;
            m_View = view;
            m_Projection = proj;
        }

        void AfterViewport() override;

    private:
        SceneWorkspace& m_SceneWorkspace;

        ImGuizmo::MODE m_GizmoMode = ImGuizmo::MODE::WORLD;
        ImGuizmo::OPERATION m_GizmoOperation = ImGuizmo::OPERATION::TRANSLATE;

        uint2 m_SelectRequestPosition;
        Request m_SelectRequest = false;

        Entity m_Selected;
        float4x4 m_View;
        float4x4 m_Projection;
    };
}

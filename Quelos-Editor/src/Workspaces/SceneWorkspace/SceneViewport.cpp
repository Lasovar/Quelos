//
// Created by lasovar on 6/6/26.
//

#include "SceneViewport.h"

#include "Workspaces/SceneWorkspace.h"
#include "UndoSystem.h"
#include "Quelos/ImGui/widgets/gizmo.h"
#include "Quelos/Scenes/Components.h"

namespace QuelosEditor {
    SceneViewport::SceneViewport(
        std::string name, SceneWorkspace& sceneWorkspace, const RenderPassHandle renderPassHandle, const uint32_t width,
        const uint32_t height
    ) : ViewportPanel(std::move(name), renderPassHandle, width, height), m_SceneWorkspace(sceneWorkspace) {
    }

    void SceneViewport::AfterViewport() {
        ImGuizmo::SetRect(
            m_ViewportBounds[0].x,
            m_ViewportBounds[0].y,
            m_ViewportSize.x,
            m_ViewportSize.y
        );

        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist();

        if (ImGui::IsKeyPressed(ImGuiKey_W, false)) {
            m_GizmoOperation = ImGuizmo::OPERATION::TRANSLATE;
        } else if (ImGui::IsKeyPressed(ImGuiKey_E, false)) {
            m_GizmoOperation = ImGuizmo::OPERATION::ROTATE;
        } else if (ImGui::IsKeyPressed(ImGuiKey_R, false)) {
            m_GizmoOperation = ImGuizmo::OPERATION::SCALE;
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Z, false)) {
            m_GizmoMode = ImGuizmo::MODE::WORLD;
        } else if (ImGui::IsKeyPressed(ImGuiKey_X, false)) {
            m_GizmoMode = ImGuizmo::MODE::LOCAL;
        }

        bool shouldSnap = false;
        if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl)) {
            shouldSnap = true;
        }

        if (m_Selected.IsValid() && m_Selected.Has<LocalTransform>()) {
            const float4x4& view = m_View;
            const float4x4& proj = m_Projection;

            WorldTransform transform = m_Selected.Get<WorldTransform>();
            LocalTransform& localTransform = m_Selected.GetMut<LocalTransform>();

            constexpr float translationSnap = 1.0f;
            constexpr float rotationSnap = 15.0f;
            constexpr float scaleSnap = 0.5f;

            const float* snapValue = nullptr;

            if (shouldSnap) {
                switch (m_GizmoOperation) {
                case ImGuizmo::UNIVERSAL:
                    break;
                case ImGuizmo::TRANSLATE_X:
                case ImGuizmo::TRANSLATE_Y:
                case ImGuizmo::TRANSLATE_Z:
                case ImGuizmo::TRANSLATE:
                    snapValue = &translationSnap;
                    break;
                case ImGuizmo::ROTATE:
                case ImGuizmo::ROTATE_X:
                case ImGuizmo::ROTATE_Y:
                case ImGuizmo::ROTATE_Z:
                case ImGuizmo::ROTATE_SCREEN:
                    snapValue = &rotationSnap;
                    break;
                case ImGuizmo::SCALE_X:
                case ImGuizmo::SCALE_Y:
                case ImGuizmo::SCALE_Z:
                case ImGuizmo::SCALE_XU:
                case ImGuizmo::SCALE_YU:
                case ImGuizmo::SCALE_ZU:
                case ImGuizmo::SCALEU:
                case ImGuizmo::SCALE:
                    snapValue = &scaleSnap;
                    break;
                case ImGuizmo::BOUNDS:
                    break;
                }
            }

            ImGuizmo::Manipulate(
                math::value_ptr(view),
                math::value_ptr(proj),
                m_GizmoOperation,
                m_GizmoMode,
                math::value_ptr(transform.Value),
                nullptr,
                snapValue
            );

            if (const Entity parent = m_Selected.GetParent(); parent.IsValid()) {
                if (const WorldTransform* parentWorld = parent.TryGet<WorldTransform>()) {
                    transform.Value = math::mul(math::inverse(parentWorld->Value), transform.Value);
                }
            }

            static LocalTransform startValue;
            static bool isEditing = false;

            // decompose localMatrix instead
            if (ImGuizmo::IsUsing()) {
                float3 translation;
                quaternion eulerRotation;
                float3 scale;

                math::decompose(
                    transform.Value,
                    translation,
                    eulerRotation,
                    scale
                );

                localTransform.Position = translation;
                localTransform.Rotation = eulerRotation;
                localTransform.Scale = scale;

                if (!isEditing) {
                    startValue = localTransform;
                    isEditing = true;
                }
            }
            else if (isEditing) {
                const auto& scene = m_SceneWorkspace.GetScene();
                ComponentSnapshot before = ComponentSnapshot::Create(scene, startValue);
                ComponentSnapshot after = ComponentSnapshot::Create(scene, localTransform);

                UndoSystem::Get().Push<SetComponentCommand>(
                    scene,
                    m_Selected.Get<EntityID>(),
                    std::move(before),
                    std::move(after)
                );

                isEditing = false;
            }
        }

        ImVec2 mousePos = ImGui::GetMousePos();

        float2 relativeMousePos = {
            mousePos.x - m_ViewportBounds[0].x,
            mousePos.y - m_ViewportBounds[0].y
        };

        // Clamp to viewport bounds to make sure click is inside
        bool insideViewport = relativeMousePos.x >= 0 && relativeMousePos.y >= 0
                           && relativeMousePos.x < m_ViewportSize.x
                           && relativeMousePos.y < m_ViewportSize.y;

        if (!ImGuizmo::IsUsing() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && m_ViewportHovered && insideViewport) {
            m_SelectRequestPosition = {
                static_cast<uint32_t>(relativeMousePos.x),
                static_cast<uint32_t>(relativeMousePos.y)
            };

            m_SelectRequest = true;
        }
    }
}

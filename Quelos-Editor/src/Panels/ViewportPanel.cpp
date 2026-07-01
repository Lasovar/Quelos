#include "qspch.h"
#include "ViewportPanel.h"

#include "EditorUI.h"
#include "Quelos/ImGui/widgets/texture.h"
#include "imgui_internal.h"
#include "UndoSystem.h"
#include "Quelos/Renderer/Renderer.h"

#include "magic_enum/magic_enum.hpp"

using namespace magic_enum::bitwise_operators;

namespace QuelosEditor {
    ViewportPanel::ViewportPanel(
        std::string name, WorldRendererView&& worldRendererView, const uint32_t width, const uint32_t height
    )
        : m_Name(std::move(name)) {
        m_ViewportSize = { width, height };
        SetWorldRendererView(std::move(worldRendererView));
    }

    void ViewportPanel::SetWorldRendererView(WorldRendererView&& worldRendererView) {
        m_WorldRendererView = std::move(worldRendererView);
    }

    bool ViewportPanel::ResizeIfNeeded() {
        if (!m_NeedResize) {
            return false;
        }

        m_ViewportSize = m_ViewportNewSize;

        WorldRenderer::ResizeView(
            m_WorldRendererView,
            { static_cast<uint32_t>(m_ViewportSize.x), static_cast<uint32_t>(m_ViewportSize.y) }
        );

        m_NeedResize = false;

        return true;
    }

    void ViewportPanel::OnImGuiRender(const ImGuiID dockspaceID, const ImGuiWindowClass& windowClass) {
        constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

        m_ViewportVisible = false;

        if (m_IsEnabled) {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.0f, 0.0f});
            if (UI::Begin(m_Name, dockspaceID, windowClass, &m_IsEnabled, flags)) {
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {0, 0});
                BeforeViewport();

                const auto viewportOffset = ImGui::GetCursorPos(); // Includes the tab bar

                ImVec2 viewPortPanelSize = ImGui::GetContentRegionAvail();

                if (!math::approximately(m_ViewportSize, float2(viewPortPanelSize.x, viewPortPanelSize.y))) {
                    QueueResize(viewPortPanelSize.x, viewPortPanelSize.y);
                    //m_ActiveScene->OnViewportResize(static_cast<uint32_t>(m_ViewportSize.x), static_cast<uint32_t>(m_ViewportSize.y));
                }

                float4 uv = /*m_ColorAttachment->IsVFlipped()
                                   ? float4(0.0f, 1.0f, 1.0f, 0.0f)
                                   : */float4(0.0f, 0.0f, 1.0f, 1.0f);

                ImGui::Image(
                    TextureHandle(m_WorldRendererView.SceneColor.GetHandle()).GetNativeHandle(),
                    {m_ViewportSize.x, m_ViewportSize.y},
                    {uv.x, uv.y},
                    {uv.z, uv.w}
                );
                ImGui::PopStyleVar();

                ImVec2 minBound = ImGui::GetWindowPos();
                minBound.x += viewportOffset.x;
                minBound.y += viewportOffset.y;

                ImVec2 maxBound = {minBound.x + viewPortPanelSize.x, minBound.y + viewPortPanelSize.y};
                m_ViewportBounds[0] = {minBound.x, minBound.y};
                m_ViewportBounds[1] = {maxBound.x, maxBound.y};

                m_ViewportFocused = ImGui::IsWindowFocused();
                m_ViewportHovered = ImGui::IsWindowHovered();

                const ImGuiDockNode* node = ImGui::GetWindowDockNode();
                m_ViewportVisible = !node /* floating window (supposedly) */ ? true : node->IsVisible;

                AfterViewport();
            }
            UI::End();

            ImGui::PopStyleVar(1);
        }
    }

    void ViewportPanel::QueueResize(const float width, const float height) {
        if (width < 1 || height < 1 || (width == m_ViewportSize.x && height == m_ViewportSize.y)) {
            return;
        }

        m_ViewportNewSize = {width, height};
        m_NeedResize = true;
    }
}

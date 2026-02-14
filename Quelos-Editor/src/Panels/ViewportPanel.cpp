#include "qspch.h"
#include "ViewportPanel.h"

#include "Quelos/ImGui/widgets/texture.h"
#include "imgui.h"
#include "imgui_internal.h"

namespace Quelos {
    ViewportPanel::ViewportPanel(const std::string& name, const uint32_t viewId, const uint32_t width, const uint32_t height)
        : m_Name(name)
    {
        auto colorSpec = TextureSpecification{};
        colorSpec.Width = width;
        colorSpec.Height = height;

        colorSpec.Format = ImageFormat::RGBA;
        colorSpec.SamplerWrap = TextureWrap::Clamp;

        colorSpec.IsRenderTarget = true;

        m_ColorAttachment = Texture2D::Create(colorSpec);

        auto depthSpec = TextureSpecification{};
        depthSpec.Width = width;
        depthSpec.Height = height;

        depthSpec.Format = ImageFormat::Depth;
        depthSpec.SamplerWrap = TextureWrap::Repeat;

        depthSpec.IsRenderTarget = true;

        m_DepthAttachment = Texture2D::Create(depthSpec);

        m_FrameBuffer = FrameBuffer::CreateFrameBuffer(viewId, {m_ColorAttachment, m_DepthAttachment});
    }

    bool ViewportPanel::ResizeIfNeeded() {
        if (!m_NeedResize) {
            return false;
        }

        m_ViewportSize = m_ViewportNewSize;

        m_FrameBuffer->Resize(m_ViewportSize.x, m_ViewportSize.y);
        m_NeedResize = false;

        QS_INFO("Viewport resized to: {}x{}", m_ViewportNewSize.x, m_ViewportNewSize.y);
        return true;
    }

    void ViewportPanel::OnImGuiRender(const ImGuiID dockspaceID, const ImGuiWindowClass& windowClass) {
        ImGui::SetNextWindowDockID(dockspaceID, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowClass(&windowClass);

        constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

        m_ViewportVisible = false;

        if (m_IsEnabled) {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.0f, 0.0f});
            if (!ImGui::Begin(m_Name.c_str(), &m_IsEnabled, flags)) {
                ImGui::PopStyleVar(1);
                ImGui::End();
            }
            else {
                const auto viewportOffset = ImGui::GetCursorPos(); // Includes the tab bar

                ImVec2 viewPortPanelSize = ImGui::GetContentRegionAvail();

                if (m_ViewportSize != *reinterpret_cast<glm::vec2*>(&viewPortPanelSize)) {
                    QueueResize(viewPortPanelSize.x, viewPortPanelSize.y);
                    //m_ActiveScene->OnViewportResize(static_cast<uint32_t>(m_ViewportSize.x), static_cast<uint32_t>(m_ViewportSize.y));
                }

                ImGui::Image(
                    m_ColorAttachment,
                    {m_ViewportSize.x, m_ViewportSize.y},
                    {0.0f, 1.0f},
                    {1.0f, 0.0f}
                );

                const auto windowSize = ImGui::GetContentRegionAvail();
                ImVec2 minBound = ImGui::GetWindowPos();
                minBound.x += viewportOffset.x;
                minBound.y += viewportOffset.y;

                ImVec2 maxBound = {minBound.x + windowSize.x, minBound.y + windowSize.y};
                m_ViewportBounds[0] = {minBound.x, minBound.y};
                m_ViewportBounds[1] = {maxBound.x, maxBound.y};

                m_ViewportFocused = ImGui::IsWindowFocused();
                m_ViewportHovered = ImGui::IsWindowHovered();

                const ImGuiDockNode* node = ImGui::GetWindowDockNode();
                m_ViewportVisible = !node /* floating window (supposedly) */ ? true : node->IsVisible;

                ImGui::End();
                ImGui::PopStyleVar();
            }
        }
    }

    void ViewportPanel::QueueResize(const float width, const float height) {
        if (width == 0 || height == 0 || (width == m_ViewportSize.x && height == m_ViewportSize.y)) {
            return;
        }

        m_ViewportNewSize = {width, height};
        m_NeedResize = true;
    }
}

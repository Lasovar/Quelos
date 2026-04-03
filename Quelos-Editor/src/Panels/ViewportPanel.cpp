#include "qspch.h"
#include "ViewportPanel.h"

#include "EditorUI.h"
#include "Quelos/ImGui/widgets/texture.h"
#include "imgui.h"
#include "imgui_internal.h"

namespace QuelosEditor {
    ViewportPanel::ViewportPanel(std::string  name, const uint32_t viewId, const uint32_t width, const uint32_t height)
        : m_Name(std::move(name))
    {
        {
            TextureSpecification colorSpec;
            colorSpec.Width = width;
            colorSpec.Height = height;

            colorSpec.Format = ImageFormat::RGBA;
            colorSpec.SamplerWrap = TextureWrap::Clamp;

            colorSpec.RenderTarget = TextureRenderTarget::ReadWrite;
            colorSpec.MSAAType = RenderTargetMSAA::MSAA_X4;

            m_ColorAttachment = Texture2D::Create(colorSpec);

            TextureSpecification depthSpec;
            depthSpec.Width = width;
            depthSpec.Height = height;

            depthSpec.Format = ImageFormat::Depth;
            depthSpec.SamplerWrap = TextureWrap::Repeat;

            depthSpec.RenderTarget = TextureRenderTarget::WriteOnly;
            depthSpec.MSAAType = RenderTargetMSAA::MSAA_X4;

            m_DepthAttachment = Texture2D::Create(depthSpec);

            m_FrameBuffer = FrameBuffer::CreateFrameBuffer(viewId, {m_ColorAttachment, m_DepthAttachment});
        }
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
        constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

        m_ViewportVisible = false;

        if (m_IsEnabled) {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.0f, 0.0f});
            if (UI::Begin(m_Name, dockspaceID, windowClass, &m_IsEnabled, flags)) {
                const auto viewportOffset = ImGui::GetCursorPos(); // Includes the tab bar

                ImVec2 viewPortPanelSize = ImGui::GetContentRegionAvail();

                if (m_ViewportSize != *reinterpret_cast<glm::vec2*>(&viewPortPanelSize)) {
                    QueueResize(viewPortPanelSize.x, viewPortPanelSize.y);
                    //m_ActiveScene->OnViewportResize(static_cast<uint32_t>(m_ViewportSize.x), static_cast<uint32_t>(m_ViewportSize.y));
                }

                glm::vec4 uv = m_ColorAttachment->IsVFlipped()
                                   ? glm::vec4(0.0f, 1.0f, 1.0f, 0.0f)
                                   : glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);

                ImGui::Image(
                    m_ColorAttachment,
                    {m_ViewportSize.x, m_ViewportSize.y},
                    {uv.x, uv.y},
                    {uv.z, uv.w}
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
            } UI::End();

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

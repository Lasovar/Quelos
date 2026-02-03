#pragma once

#include "Quelos/Renderer/FrameBuffer.h"

#include "imgui.h"

namespace Quelos {
    class ViewportPanel {
    public:
        ViewportPanel() = default;
        ViewportPanel(uint32_t width, uint32_t height);

        [[nodiscard]] Ref<FrameBuffer> GetFrameBuffer() { return m_FrameBuffer; }
        [[nodiscard]] Ref<FrameBuffer> GetFrameBuffer() const { return m_FrameBuffer; }
        void ResizeIfNeeded();

        void OnImGuiRender(ImGuiID dockspaceID, const ImGuiWindowClass& windowClass);
    private:
        void QueueResize(float width, float height);
    private:
        bool m_IsEnabled = true;

        bool m_ViewportFocused = false;
        bool m_ViewportHovered = false;

        glm::vec2 m_ViewportBounds[2] = { glm::vec2(0.0f), glm::vec2(0.0f) };

        bool m_NeedResize = false;

        glm::vec2 m_ViewportNewSize{};
        glm::vec2 m_ViewportSize{};

        Ref<Texture2D> m_ColorAttachment;
        Ref<Texture2D> m_DepthAttachment;
        Ref<FrameBuffer> m_FrameBuffer;
    };
}

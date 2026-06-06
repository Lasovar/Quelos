#pragma once

#include "Quelos/Renderer/FrameBuffer.h"

#include "imgui.h"
#include "Quelos/Scenes/Entity.h"

namespace QuelosEditor {
    using namespace Quelos;
    class ViewportPanel {
    public:
        ViewportPanel() = default;
        ViewportPanel(std::string name, RenderPassHandle renderPassHandle, uint32_t width, uint32_t height);

        virtual ~ViewportPanel() = default;

        [[nodiscard]] Ref<FrameBuffer> GetFrameBuffer() { return m_FrameBuffer; }
        [[nodiscard]] Ref<FrameBuffer> GetFrameBuffer() const { return m_FrameBuffer; }

        bool ResizeIfNeeded();

        [[nodiscard]] float2 GetViewportSize() const { return m_ViewportSize; }

        [[nodiscard]] bool IsViewportFocused() const { return m_ViewportFocused; }
        [[nodiscard]] bool IsViewportHovered() const { return m_ViewportHovered; }

        void OnImGuiRender(ImGuiID dockspaceID, const ImGuiWindowClass& windowClass);

        virtual void BeforeViewport() {}
        virtual void AfterViewport() {}

        [[nodiscard]] bool ShouldDraw() const { return m_ViewportVisible; }
    private:
        void QueueResize(float width, float height);
    protected:
        std::string m_Name;
        bool m_IsEnabled = true;

        bool m_ViewportFocused = false;
        bool m_ViewportHovered = false;

        float2 m_ViewportBounds[2] = { float2(0.0f), float2(0.0f) };

        bool m_NeedResize = false;

        float2 m_ViewportNewSize{};
        float2 m_ViewportSize{};

        bool m_ViewportVisible = false;

        TextureHandle m_ColorAttachment;
        TextureHandle m_DepthAttachment;
        TextureHandle m_SceneColorAttachment;

        Ref<FrameBuffer> m_FrameBuffer;
    };
}

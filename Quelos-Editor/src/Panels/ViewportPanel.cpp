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
        std::string name, const RenderPassHandle renderPassHandle, const RenderPassHandle shadowMaskPass, const uint32_t width, const uint32_t height
    )
        : m_Name(std::move(name)) {
        m_ViewportSize = { width, height };
        SetRenderPass(renderPassHandle, shadowMaskPass);
    }

    void ViewportPanel::SetRenderPass(const RenderPassHandle gBufferPass, const RenderPassHandle shadowMaskPass) {
        TextureSpecification msaaColorSpec;
        msaaColorSpec.Width = m_ViewportSize.x;
        msaaColorSpec.Height = m_ViewportSize.y;

        msaaColorSpec.Format = ImageFormat::RGBA8UNorm;
        msaaColorSpec.SamplerWrap = WrapMode::Clamp;

        msaaColorSpec.BindFlags = Bind::RenderTarget;
        msaaColorSpec.SampleCount = SampleCount::x4;

        m_ColorAttachment = Renderer::CreateTexture(msaaColorSpec);

        TextureSpecification sceneColor;
        sceneColor.Width = m_ViewportSize.x;
        sceneColor.Height = m_ViewportSize.y;

        sceneColor.Format = ImageFormat::RGBA8UNorm;
        sceneColor.SamplerWrap = WrapMode::Repeat;

        sceneColor.BindFlags = Bind::RenderTarget | Bind::ShaderResource;
        sceneColor.SampleCount = SampleCount::x1;

        m_SceneColorAttachment = Renderer::CreateTexture(sceneColor);

        TextureSpecification msaaDepthSpec;
        msaaDepthSpec.Width = m_ViewportSize.x;
        msaaDepthSpec.Height = m_ViewportSize.y;

        msaaDepthSpec.Format = ImageFormat::Depth32Float;
        msaaDepthSpec.SamplerWrap = WrapMode::Repeat;

        msaaDepthSpec.BindFlags = Bind::DepthStencil | Bind::ShaderResource;
        msaaDepthSpec.SampleCount = SampleCount::x4;

        m_DepthAttachment = Renderer::CreateTexture(msaaDepthSpec);

        const TextureViewHandle attachments[] = {
            Renderer::TextureGetDefaultView(m_ColorAttachment.GetHandle(), TextureViewType::RenderTarget),
            Renderer::TextureGetDefaultView(m_SceneColorAttachment.GetHandle(), TextureViewType::RenderTarget),
            Renderer::TextureGetDefaultView(m_DepthAttachment.GetHandle(), TextureViewType::DepthStencil)
        };

        FrameBufferSpec spec;
        spec.Attachments = attachments;
        spec.Name = m_Name;
        spec.RenderPassHandle = gBufferPass;
        spec.Size = {static_cast<uint32_t>(m_ViewportSize.x), static_cast<uint32_t>(m_ViewportSize.y)};

        m_FrameBuffer = FrameBuffer::Create(spec);
        m_NeedResize = true;

        TextureSpecification maskTextureSpec;
        maskTextureSpec.Type = TextureType::Texture2D;
        maskTextureSpec.Format = ImageFormat::R8UNorm;
        maskTextureSpec.Width = 1;
        maskTextureSpec.Height = 1;
        maskTextureSpec.BindFlags = Bind::RenderTarget | Bind::ShaderResource;

        m_ShadowMaskTexture = Renderer::CreateTexture(maskTextureSpec);

        TextureViewHandle maskRTV = Renderer::TextureGetDefaultView(
            m_ShadowMaskTexture.GetHandle(),
            TextureViewType::RenderTarget
        );

        FrameBufferSpec fbDesc;
        fbDesc.Name = "ShadowMaskFB";
        fbDesc.RenderPassHandle = shadowMaskPass;
        fbDesc.Attachments = Span32(&maskRTV, 1);
        fbDesc.Size = { 1, 1 };

        m_ShadowMaskFB = Renderer::CreateFrameBuffer(fbDesc);
    }

    bool ViewportPanel::ResizeIfNeeded() {
        if (!m_NeedResize) {
            return false;
        }

        m_ViewportSize = m_ViewportNewSize;

        Renderer::TextureResize(m_SceneColorAttachment.GetHandle(), m_ViewportSize.x, m_ViewportSize.y);
        Renderer::TextureResize(m_DepthAttachment.GetHandle(), m_ViewportSize.x, m_ViewportSize.y);
        Renderer::TextureResize(m_ColorAttachment.GetHandle(), m_ViewportSize.x, m_ViewportSize.y);
        m_FrameBuffer->Resize(m_ViewportSize.x, m_ViewportSize.y);

        Renderer::TextureResize(m_ShadowMaskTexture.GetHandle(), m_ViewportSize.x, m_ViewportSize.y);
        Renderer::FrameBufferResize(m_ShadowMaskFB.GetHandle(), m_ViewportSize.x, m_ViewportSize.y);

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
                    TextureHandle(m_SceneColorAttachment.GetHandle()).GetNativeHandle(),
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

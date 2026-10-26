//
// Created by lasovar on 10/25/26.
//

#include "TextureWorkspace.hpp"

#include "EditorUI.h"
#include "Quelos/Utility/FileSystem.h"

#include "imgui_internal.h"
#include "AssetManagement/EditorAssetManager.h"

#include "magic_enum/magic_enum.hpp"
using namespace magic_enum::bitwise_operators;

namespace QuelosEditor {
    TextureWorkspace::TextureWorkspace(UndoSystem& undoSystem, const AssetMetadata& metadata)
        : Workspace(std::string(FS::Filename(metadata.FilePath)), undoSystem),
          m_Texture(metadata.Handle),
          m_PropertiesPanel(undoSystem, m_Texture)
    {
        m_WorkspaceNameHash = metadata.Handle.ToString(Allocator::Persistent);
        m_WorkspaceID = ImHashStr(FormatTemp("{}_TextureDockspace", m_WorkspaceName));

        Texture& texture = m_Texture.Get();
        const TextureSpec& textureSpec = texture.GetSpec();

        m_TextureAspectRatio = static_cast<float>(textureSpec.Width) / static_cast<float>(textureSpec.Height);
        m_TextureSRV = Renderer::TextureGetDefaultView(texture.GetHandle(), TextureViewType::ShaderResource);

        TextureSpec previewSpec;
        previewSpec.Type = TextureType::Texture2D;
        previewSpec.Format = TextureFormat::RGBA8UNorm;
        previewSpec.SampleCount = SampleCount::x1;
        previewSpec.BindFlags = Bind::RenderTarget | Bind::ShaderResource;
        previewSpec.Width = textureSpec.Width;
        previewSpec.Height = textureSpec.Height;
        previewSpec.Name = FormatTemp("{}_WorkspacePreview", textureSpec.Name);

        m_PreviewTexture = Renderer::CreateTexture(previewSpec);
        m_PreviewRTV = Renderer::TextureGetDefaultView(m_PreviewTexture, TextureViewType::RenderTarget);
        m_PreviewSRV = Renderer::TextureGetDefaultView(m_PreviewTexture, TextureViewType::ShaderResource);

        RenderPassAttachmentSpec attachmentSpec;
        attachmentSpec.Format = TextureFormat::RGBA8UNorm;
        attachmentSpec.LoadOp = AttachmentLoadOp::Clear;
        attachmentSpec.StoreOp = AttachmentStoreOp::Store;
        attachmentSpec.SampleCount = 1;
        attachmentSpec.InitialState = ResourceState::RenderTarget;
        attachmentSpec.FinalState = ResourceState::ShaderResource;

        AttachmentReference attachmentReference = { .AttachmentIndex = 0, .State = ResourceState::RenderTarget };

        SubPassSpec subPassSpec;
        subPassSpec.RenderTargetAttachments = Span(&attachmentReference, 1);

        RenderPassSpec renderPassSpec;
        String rpName(FormatTemp("{}_WorkspacePreviewRP", FS::Stem(metadata.FilePath)), Allocator::Frame);
        renderPassSpec.Name = rpName;
        renderPassSpec.Attachments = Span(&attachmentSpec, 1);
        renderPassSpec.SubPasses = Span(&subPassSpec, 1);

        m_PreviewRP = Renderer::CreateRenderPass(renderPassSpec);

        FrameBufferSpec frameBufferSpec;
        String fbName(FormatTemp("{}_WorkspacePreviewFB", FS::Stem(metadata.FilePath)), Allocator::Frame);
        frameBufferSpec.Name = fbName;
        frameBufferSpec.Attachments = Span(&m_PreviewRTV, 1);
        frameBufferSpec.RenderPassHandle = m_PreviewRP;
        frameBufferSpec.Size = { .Width = textureSpec.Width, .Height = textureSpec.Height };

        m_PreviewFB = Renderer::CreateFrameBuffer(frameBufferSpec);

        CreatePSO();
    }

    void TextureWorkspace::Tick(float) {
        if (!Renderer::IsAlive(m_PreviewPSO)) {
            Renderer::Destroy(m_PreviewSRB);
            CreatePSO();
        }

        if (m_NeedResize.Resolve()) {
            m_ViewportSize = m_ViewportNewSize;

            Renderer::TextureResize(m_PreviewTexture, m_ViewportSize.x, m_ViewportSize.y);
            Renderer::FrameBufferResize(m_PreviewFB, m_ViewportSize.x, m_ViewportSize.y);

            Renderer::Destroy(m_PreviewSRB);

            m_PreviewSRB = Renderer::CreateShaderResourceBinding(m_PreviewPSO, true);
            Renderer::BindVariableByName(
                ShaderType::Fragment,
                m_PreviewSRB,
                "_SourceTexture",
                m_TextureSRV,
                SetShaderResourceFlag::None
            );

            m_RedrawPreview = true;
        }

        if (m_RedrawPreview.Resolve()) {
            BeginRenderPassAttribs attribs;
            attribs.RenderPassHandle = m_PreviewRP;
            attribs.FrameBufferHandle = m_PreviewFB;
            ClearValue clearValue[1] = {
                { .Format = TextureFormat::RGBA8UNorm, .Color = Color::Red() }
            };

            attribs.ClearColors = clearValue;

            Renderer::BindPipelineState(m_PreviewPSO);
            Renderer::CommitShaderResources(m_PreviewSRB, ResourceStateTransitionMode::Transition);

            Renderer::BeginRenderPass(attribs);

            DrawAttribs drawAttribs;
            drawAttribs.NumVertices = 3;
            Renderer::Draw(drawAttribs);

            Renderer::EndRenderPass();
        }
    }

    void TextureWorkspace::WorkspaceContents() {
        m_PropertiesPanel.OnImGuiRender(m_WorkspaceID, m_WorkspaceClass);

        constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.0f, 0.0f});
        if (UI::Begin("Preview", m_WorkspaceID, m_WorkspaceClass, nullptr, flags) && m_Texture) {
            float4 uv(0.0f, 0.0f, 1.0f, 1.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {0, 0});

            const ImVec2 regionOffset = ImGui::GetCursorPos(); // includes tab bar
            const ImVec2 avail = ImGui::GetContentRegionAvail();

            if (avail.x >= 1.0f && avail.y >= 1.0f && m_TextureAspectRatio > 0.0f) {
                // Fit: limited by height if the region is wider than the texture, else by width
                float2 fitted;
                if (avail.x / avail.y > m_TextureAspectRatio) {
                    fitted = {avail.y * m_TextureAspectRatio, avail.y};
                }
                else {
                    fitted = {avail.x, avail.x / m_TextureAspectRatio};
                }

                fitted = math::max(math::floor(fitted), float2(1.0f));   // whole pixels, never 0

                const ImVec2 centered = {
                    regionOffset.x + math::floor((avail.x - fitted.x) * 0.5f),
                    regionOffset.y + math::floor((avail.y - fitted.y) * 0.5f)
                };

                if (!math::approximately(m_ViewportSize, fitted)) {
                    QueueResize(fitted.x, fitted.y);
                }

                ImGui::SetCursorPos(centered);
                ImGui::Image(
                    m_PreviewTexture.GetNativeHandle(),
                    {m_ViewportSize.x, m_ViewportSize.y},
                    {uv.x, uv.y},
                    {uv.z, uv.w}
                );

                const ImVec2 winPos = ImGui::GetWindowPos();
                m_ViewportBounds[0] = {winPos.x + centered.x, winPos.y + centered.y};
                m_ViewportBounds[1] = {m_ViewportBounds[0].x + fitted.x, m_ViewportBounds[0].y + fitted.y};
            }

            ImGui::PopStyleVar();
        }
        UI::End();

        ImGui::PopStyleVar(1);
    }

    void TextureWorkspace::QueueResize(const float width, const float height) {
        if (width < 1 || height < 1 || (width == m_ViewportSize.x && height == m_ViewportSize.y)) {
            return;
        }

        m_ViewportNewSize = {width, height};
        m_NeedResize = true;
    }

    void TextureWorkspace::CreatePSO() {
        const TextureSpec& textureSpec = m_Texture.Get().GetSpec();

        const auto& assetManager = SharedAs<EditorAssetManager>(Project::GetAssetManager());
        const AssetMetadata& shaderMetadata = *assetManager->GetAssetMetadata("Assets/shaders/texture_preview.slang");

        m_PreviewShader = AssetRef<GraphicsShader>(shaderMetadata.Handle);
        GraphicsShader& shader = m_PreviewShader.Get();
        const GraphicsShaderPass& passInfo = *shader.GetShaderPass("Preview");

        GraphicsPipelineStateCreateInfo psoCI;
        psoCI.Name = "PreviewPSO";
        psoCI.VertexShader = passInfo.Pipelines[0].VertexShader;
        psoCI.FragmentShader = passInfo.Pipelines[0].FragmentShader;
        psoCI.Spec.Type = PipelineType::Graphics;

        psoCI.GraphicsPipeline.RenderPass = m_PreviewRP;
        psoCI.GraphicsPipeline.RasterizerSpec.CullMode = CullMode::None;
        psoCI.GraphicsPipeline.DepthStencilSpec.DepthEnable = false;

        ShaderResourceVariableSpec vars[] = {
            { "_SourceTexture", ShaderType::Fragment, ShaderResourceVariableType::Mutable },
        };

        psoCI.Spec.ResourceLayout.Variables = vars;

        SamplerSpec samplerSpec;
        samplerSpec.WrapU = textureSpec.SamplerWrap;
        samplerSpec.WrapV = textureSpec.SamplerWrap;
        samplerSpec.MinFilter = textureSpec.SamplerFilter;
        samplerSpec.MagFilter = textureSpec.SamplerFilter;
        samplerSpec.MipFilter = textureSpec.SamplerFilter;

        ImmutableSamplerSpec immutableSamplers[1];
        immutableSamplers[0].SamplerOrTextureName = "_SourceTexture";
        immutableSamplers[0].Specification = samplerSpec;
        immutableSamplers[0].ShaderStages = ShaderType::Fragment;

        psoCI.Spec.ResourceLayout.ImmutableSamplers = immutableSamplers;

        m_PreviewPSO = Renderer::CreatePipelineState(psoCI);
        shader.AddPipelineState(m_PreviewPSO);

        m_PreviewSRB = Renderer::CreateShaderResourceBinding(m_PreviewPSO, true);
        Renderer::BindVariableByName(
            ShaderType::Fragment,
            m_PreviewSRB,
            "_SourceTexture",
            m_TextureSRV,
            SetShaderResourceFlag::None
        );

        m_RedrawPreview = true;
    }

    void TextureWorkspace::ReleasePSO() const {
        Renderer::Destroy(m_PreviewSRB);
        Renderer::Destroy(m_PreviewPSO);
    }

    void TextureWorkspace::OnEvent(Event&) {}

    TextureWorkspace::~TextureWorkspace() {
        ReleasePSO();
        Renderer::Destroy(m_PreviewFB);
        Renderer::Destroy(m_PreviewRP);
        Renderer::Destroy(m_PreviewTexture);
    }
} // QuelosEditor
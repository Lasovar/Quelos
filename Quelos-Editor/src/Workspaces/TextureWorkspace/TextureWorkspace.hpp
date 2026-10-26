//
// Created by lasovar on 10/25/26.
//

#pragma once
#include "TexturePropertiesPanel.hpp"
#include "Quelos/Utility/Request.h"
#include "Workspaces/Workspace.h"

namespace QuelosEditor {
    class TextureWorkspace : public Workspace {
    public:
        TextureWorkspace(UndoSystem& undoSystem, const AssetMetadata& metadata);

        void Tick(float deltaTime) override;
        void WorkspaceContents() override;
        void OnEvent(Event& event) override;

        ~TextureWorkspace() override;

    private:
        void QueueResize(float width, float height);
    private:
        AssetRef<GraphicsShader> m_PreviewShader;
        float m_TextureAspectRatio = 0.0f;

        AssetRef<Texture> m_Texture;
        TextureViewHandle m_TextureSRV;
        TextureHandle m_PreviewTexture;
        TextureViewHandle m_PreviewRTV;
        TextureViewHandle m_PreviewSRV;

        TexturePropertiesPanel m_PropertiesPanel;
        RenderPassHandle m_PreviewRP;
        FrameBufferHandle m_PreviewFB;
        PipelineStateHandle m_PreviewPSO;
        ShaderResourceBindingHandle m_PreviewSRB;

        float2 m_ViewportSize;
        float2 m_ViewportNewSize;
        Array<float2, 2> m_ViewportBounds;

        Request m_NeedResize = false;

        Request m_RedrawPreview;
    };
} // QuelosEditor

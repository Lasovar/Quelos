//
// Created by lasovar on 10/25/26.
//

#include "TextureWorkspace.hpp"

#include "EditorUI.h"
#include "Quelos/Utility/FileSystem.h"

#include "imgui_internal.h"

namespace QuelosEditor {
    TextureWorkspace::TextureWorkspace(UndoSystem& undoSystem, const AssetMetadata& metadata)
        : Workspace(std::string(FS::Filename(metadata.FilePath)), undoSystem),
          m_Texture(metadata.Handle),
          m_PropertiesPanel(undoSystem, m_Texture)
    {
        m_WorkspaceNameHash = metadata.Handle.ToString(Allocator::Persistent);
        m_WorkspaceID = ImHashStr(FormatTemp("{}_TextureDockspace", m_WorkspaceName));

        TextureViewSpec spec;
        spec.TextureType = TextureType::Texture2D;
        spec.Format = TextureFormat::RGBA8UNorm;
        spec.MipLevels = 0;
        spec.ViewType = TextureViewType::ShaderResource;
        spec.Flags = TextureViewFlags::None;

        m_TextureView = Renderer::TextureCreateView(m_Texture.Get().GetHandle(), spec);
    }

    void TextureWorkspace::Tick(float deltaTime) {}

    void TextureWorkspace::WorkspaceContents() {
        m_PropertiesPanel.OnImGuiRender(m_WorkspaceID, m_WorkspaceClass);

        if (UI::Begin("Preview", m_WorkspaceID, m_WorkspaceClass) && m_Texture) {
            float4 uv(0.0f, 1.0f, 1.0f, 0.0f);

            Texture& texture = m_Texture.Get();
            float2 size = texture.GetSize();
            float aspectRatio = size.x / size.y;
            ImVec2 availableSize = ImGui::GetContentRegionAvail();
            size.x = availableSize.x;
            size.y = availableSize.x / aspectRatio;

            ImGui::Image(
                Renderer::TextureGetNativeHandle(m_TextureView),
                {size.x, size.y},
                {uv.x, uv.y},
                {uv.z, uv.w}
            );
        }
        UI::End();
    }

    void TextureWorkspace::OnEvent(Event& event) {}
} // QuelosEditor
#pragma once

#include "imgui.h"
#include "Quelos/Renderer/Texture.h"

namespace ImGui {

    // Helper function for passing bgfx::TextureHandle to ImGui::Image.
    void Image(const Quelos::Ref<Quelos::Texture2D>& texture
        , uint8_t _flags
        , uint8_t _mip
        , const ImVec2& _size
        , const ImVec2& _uv0       = ImVec2(0.0f, 0.0f)
        , const ImVec2& _uv1       = ImVec2(1.0f, 1.0f)
        , const ImVec4& _tintCol   = ImVec4(1.0f, 1.0f, 1.0f, 1.0f)
        , const ImVec4& _borderCol = ImVec4(0.0f, 0.0f, 0.0f, 0.0f)
        );

    // Helper function for passing bgfx::TextureHandle to ImGui::Image.
    void Image(const Quelos::Ref<Quelos::Texture2D>& texture
        , const ImVec2& _size
        , const ImVec2& _uv0       = ImVec2(0.0f, 0.0f)
        , const ImVec2& _uv1       = ImVec2(1.0f, 1.0f)
        , const ImVec4& _tintCol   = ImVec4(1.0f, 1.0f, 1.0f, 1.0f)
        , const ImVec4& _borderCol = ImVec4(0.0f, 0.0f, 0.0f, 0.0f)
        );

    // Helper function for passing bgfx::TextureHandle to ImGui::ImageButton.
    bool ImageButton(const Quelos::Ref<Quelos::Texture2D>& texture
        , uint8_t _flags
        , uint8_t _mip
        , const ImVec2& _size
        , const ImVec2& _uv0     = ImVec2(0.0f, 0.0f)
        , const ImVec2& _uv1     = ImVec2(1.0f, 1.0f)
        , const ImVec4& _bgCol   = ImVec4(0.0f, 0.0f, 0.0f, 0.0f)
        , const ImVec4& _tintCol = ImVec4(1.0f, 1.0f, 1.0f, 1.0f)
        );

    // Helper function for passing bgfx::TextureHandle to ImGui::ImageButton.
    bool ImageButton(const Quelos::Ref<Quelos::Texture2D>& texture
        , const ImVec2& _size
        , const ImVec2& _uv0     = ImVec2(0.0f, 0.0f)
        , const ImVec2& _uv1     = ImVec2(1.0f, 1.0f)
        , const ImVec4& _bgCol   = ImVec4(0.0f, 0.0f, 0.0f, 0.0f)
        , const ImVec4& _tintCol = ImVec4(1.0f, 1.0f, 1.0f, 1.0f)
        );
}

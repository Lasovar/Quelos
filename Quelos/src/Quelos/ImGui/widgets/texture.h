#pragma once

#include "imgui.h"
#include "Quelos/Renderer/Texture.h"

namespace ImGui {
    QS_API void Image(
        const Quelos::Ref<Quelos::Texture2D>& texture,
        uint8_t flags,
        uint8_t mip,
        const ImVec2& size,
        const ImVec2& uv0 = ImVec2(0.0f, 0.0f),
        const ImVec2& uv1 = ImVec2(1.0f, 1.0f),
        const ImVec4& tintCol = ImVec4(1.0f, 1.0f, 1.0f, 1.0f),
        const ImVec4& borderCol = ImVec4(0.0f, 0.0f, 0.0f, 0.0f)
    );

    QS_API void Image(
        const Quelos::Ref<Quelos::Texture2D>& texture,
        const ImVec2& size,
        const ImVec2& uv0 = ImVec2(0.0f, 0.0f),
        const ImVec2& uv1 = ImVec2(1.0f, 1.0f),
        const ImVec4& tintCol = ImVec4(1.0f, 1.0f, 1.0f, 1.0f),
        const ImVec4& borderCol = ImVec4(0.0f, 0.0f, 0.0f, 0.0f)
    );

    QS_API bool ImageButton(
        const Quelos::Ref<Quelos::Texture2D>& texture,
        uint8_t flags,
        uint8_t mip,
        const ImVec2& size,
        const ImVec2& uv0 = ImVec2(0.0f, 0.0f),
        const ImVec2& uv1 = ImVec2(1.0f, 1.0f),
        const ImVec4& bgCol = ImVec4(0.0f, 0.0f, 0.0f, 0.0f),
        const ImVec4& tintCol = ImVec4(1.0f, 1.0f, 1.0f, 1.0f)
    );

    QS_API bool ImageButton(
        const Quelos::Ref<Quelos::Texture2D>& texture,
        const ImVec2& size,
        const ImVec2& uv0 = ImVec2(0.0f, 0.0f),
        const ImVec2& uv1 = ImVec2(1.0f, 1.0f),
        const ImVec4& bgCol = ImVec4(0.0f, 0.0f, 0.0f, 0.0f),
        const ImVec4& tintCol = ImVec4(1.0f, 1.0f, 1.0f, 1.0f))
    ;
}

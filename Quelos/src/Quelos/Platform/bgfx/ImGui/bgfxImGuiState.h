#pragma once

#include "Quelos/ImGui/ImGuiState.h"

#include "imgui.h"
#include "imgui_user.h"

#include "bgfx/bgfx.h"

namespace Quelos {
    class bgfxImGuiState : public ImGuiState {
    public:
        bgfxImGuiState() = default;

        void Init(float fontSize) override;
        void Destroy() override;

        void BeginFrame(uint32_t viewId) override;
        void EndFrame() override;
    private:
        void Render(ImDrawData* drawData) const;

    private:
        ImGuiContext* Context = nullptr;
        bgfx::ViewId ViewId = 255;

        bgfx::ProgramHandle m_Program = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle u_ImageLodEnabled = BGFX_INVALID_HANDLE;
        bgfx::ProgramHandle m_ImageProgram = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle s_Tex = BGFX_INVALID_HANDLE;
        bgfx::VertexLayout m_Layout;

        ImFont* m_Font[ImGui::Font::Count]{};
    };
}

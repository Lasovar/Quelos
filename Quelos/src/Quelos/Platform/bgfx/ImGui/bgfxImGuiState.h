#pragma once

#include "Quelos/ImGui/ImGuiState.h"

#include "imgui.h"
#include "imgui_user.h"

#include "bgfx/bgfx.h"

#include "bx/bx.h"

namespace ImGui {
    struct TextureBgfx {
        bgfx::TextureHandle handle;
        uint8_t flags;
        uint8_t mip;
        uint32_t unused;
    };

#define IMGUI_FLAGS_NONE        UINT8_C(0x00)
#define IMGUI_FLAGS_ALPHA_BLEND UINT8_C(0x01)

	///
	inline ImTextureID toId(bgfx::TextureHandle _handle, uint8_t _flags, uint8_t _mip)
	{
		TextureBgfx tex
		{
			.handle = _handle,
			.flags  = _flags,
			.mip    = _mip,
			.unused = 0,
		};

		return bx::bitCast<ImTextureID>(tex);
	}

	// Helper function for passing bgfx::TextureHandle to ImGui::Image.
	inline void Image(bgfx::TextureHandle _handle
		, uint8_t _flags
		, uint8_t _mip
		, const ImVec2& _size
		, const ImVec2& _uv0       = ImVec2(0.0f, 0.0f)
		, const ImVec2& _uv1       = ImVec2(1.0f, 1.0f)
		, const ImVec4& _tintCol   = ImVec4(1.0f, 1.0f, 1.0f, 1.0f)
		, const ImVec4& _borderCol = ImVec4(0.0f, 0.0f, 0.0f, 0.0f)
		)
	{
		ImageWithBg(toId(_handle, _flags, _mip), _size, _uv0, _uv1, _borderCol, _tintCol);
	}

	// Helper function for passing bgfx::TextureHandle to ImGui::Image.
	inline void Image(bgfx::TextureHandle _handle
		, const ImVec2& _size
		, const ImVec2& _uv0       = ImVec2(0.0f, 0.0f)
		, const ImVec2& _uv1       = ImVec2(1.0f, 1.0f)
		, const ImVec4& _tintCol   = ImVec4(1.0f, 1.0f, 1.0f, 1.0f)
		, const ImVec4& _borderCol = ImVec4(0.0f, 0.0f, 0.0f, 0.0f)
		)
	{
		Image(_handle, IMGUI_FLAGS_ALPHA_BLEND, 0, _size, _uv0, _uv1, _tintCol, _borderCol);
	}

	// Helper function for passing bgfx::TextureHandle to ImGui::ImageButton.
	inline bool ImageButton(bgfx::TextureHandle _handle
		, uint8_t _flags
		, uint8_t _mip
		, const ImVec2& _size
		, const ImVec2& _uv0     = ImVec2(0.0f, 0.0f)
		, const ImVec2& _uv1     = ImVec2(1.0f, 1.0f)
		, const ImVec4& _bgCol   = ImVec4(0.0f, 0.0f, 0.0f, 0.0f)
		, const ImVec4& _tintCol = ImVec4(1.0f, 1.0f, 1.0f, 1.0f)
		)
	{
		return ImageButton("image", toId(_handle, _flags, _mip), _size, _uv0, _uv1, _bgCol, _tintCol);
	}

	// Helper function for passing bgfx::TextureHandle to ImGui::ImageButton.
	inline bool ImageButton(bgfx::TextureHandle _handle
		, const ImVec2& _size
		, const ImVec2& _uv0     = ImVec2(0.0f, 0.0f)
		, const ImVec2& _uv1     = ImVec2(1.0f, 1.0f)
		, const ImVec4& _bgCol   = ImVec4(0.0f, 0.0f, 0.0f, 0.0f)
		, const ImVec4& _tintCol = ImVec4(1.0f, 1.0f, 1.0f, 1.0f)
		)
	{
		return ImageButton(_handle, IMGUI_FLAGS_ALPHA_BLEND, 0, _size, _uv0, _uv1, _bgCol, _tintCol);
	}
}

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

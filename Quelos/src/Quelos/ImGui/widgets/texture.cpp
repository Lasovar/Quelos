#include "qspch.h"
#include "texture.h"

namespace ImGui {
    struct QuelosTextureBgfx {
        uint16_t handle;
        uint8_t flags;
        uint8_t mip;
        uint32_t unused;
    };

#define IMGUI_FLAGS_NONE        UINT8_C(0x00)
#define IMGUI_FLAGS_ALPHA_BLEND UINT8_C(0x01)

	ImTextureID toId(const Quelos::Ref<Quelos::Texture>& texture, uint8_t _flags, uint8_t _mip) {
		const uint16_t handle = texture->GetTextureHandle();

		const QuelosTextureBgfx tex {
			.handle = handle,
			.flags  = _flags,
			.mip    = _mip,
			.unused = 0,
		};

		return std::bit_cast<ImTextureID>(tex);
	}

	// Helper function for passing bgfx::TextureHandle to ImGui::Image.
	void Image(const Quelos::Ref<Quelos::Texture2D>& texture
		, uint8_t _flags
		, uint8_t _mip
		, const ImVec2& _size
		, const ImVec2& _uv0
		, const ImVec2& _uv1
		, const ImVec4& _tintCol
		, const ImVec4& _borderCol
		)
	{
		ImageWithBg(toId(texture, _flags, _mip), _size, _uv0, _uv1, _borderCol, _tintCol);
	}

	// Helper function for passing bgfx::TextureHandle to ImGui::Image.
	void Image(const Quelos::Ref<Quelos::Texture2D>& texture
		, const ImVec2& _size
		, const ImVec2& _uv0
		, const ImVec2& _uv1
		, const ImVec4& _tintCol
		, const ImVec4& _borderCol
		)
	{
		Image(texture, IMGUI_FLAGS_ALPHA_BLEND, 0, _size, _uv0, _uv1, _tintCol, _borderCol);
	}

	// Helper function for passing bgfx::TextureHandle to ImGui::ImageButton.
	bool ImageButton(const Quelos::Ref<Quelos::Texture2D>& texture
		, uint8_t _flags
		, uint8_t _mip
		, const ImVec2& _size
		, const ImVec2& _uv0
		, const ImVec2& _uv1
		, const ImVec4& _bgCol
		, const ImVec4& _tintCol
		)
	{
		return ImageButton("image", toId(texture, _flags, _mip), _size, _uv0, _uv1, _bgCol, _tintCol);
	}

	// Helper function for passing bgfx::TextureHandle to ImGui::ImageButton.
	bool ImageButton(const Quelos::Ref<Quelos::Texture2D>& texture
		, const ImVec2& _size
		, const ImVec2& _uv0
		, const ImVec2& _uv1
		, const ImVec4& _bgCol
		, const ImVec4& _tintCol
		)
	{
		return ImageButton(texture, IMGUI_FLAGS_ALPHA_BLEND, 0, _size, _uv0, _uv1, _bgCol, _tintCol);
	}
}

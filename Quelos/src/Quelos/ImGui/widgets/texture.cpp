#include "qspch.h"
#include "texture.h"

#define IMGUI_FLAGS_NONE        UINT8_C(0x00)
#define IMGUI_FLAGS_ALPHA_BLEND UINT8_C(0x01)

namespace ImGui {
    struct QuelosTexture {
        uint16_t handle;
        uint8_t flags;
        uint8_t mip;
        uint32_t unused;
    };

	ImTextureID toId(const Quelos::Ref<Quelos::Texture>& texture, const uint8_t _flags, const uint8_t _mip) {
		const uint16_t handle = texture->GetTextureHandle();

		const QuelosTexture tex {
			.handle = handle,
			.flags  = _flags,
			.mip    = _mip,
			.unused = 0,
		};

		return std::bit_cast<ImTextureID>(tex);
	}

	void Image(
		const Quelos::Ref<Quelos::Texture2D>& texture,
		const uint8_t flags,
		const uint8_t mip,
		const ImVec2& size,
		const ImVec2& uv0,
		const ImVec2& uv1,
		const ImVec4& tintCol,
		const ImVec4& borderCol
	) {
		ImageWithBg(toId(texture, flags, mip), size, uv0, uv1, borderCol, tintCol);
	}

	void Image(
		const Quelos::Ref<Quelos::Texture2D>& texture,
		const ImVec2& size,
		const ImVec2& uv0,
		const ImVec2& uv1,
		const ImVec4& tintCol,
		const ImVec4& borderCol
	) {
		Image(texture, IMGUI_FLAGS_ALPHA_BLEND, 0, size, uv0, uv1, tintCol, borderCol);
	}

	bool ImageButton(
		const Quelos::Ref<Quelos::Texture2D>& texture,
		const uint8_t flags,
		const uint8_t mip,
		const ImVec2& size,
		const ImVec2& uv0,
		const ImVec2& uv1,
		const ImVec4& bgCol,
		const ImVec4& tintCol
	) {
		return ImageButton("image", toId(texture, flags, mip), size, uv0, uv1, bgCol, tintCol);
	}

	bool ImageButton(
		const Quelos::Ref<Quelos::Texture2D>& texture,
		const ImVec2& size,
		const ImVec2& uv0,
		const ImVec2& uv1,
		const ImVec4& bgCol,
		const ImVec4& tintCol
	) {
		return ImageButton(texture, IMGUI_FLAGS_ALPHA_BLEND, 0, size, uv0, uv1, bgCol, tintCol);
	}
}

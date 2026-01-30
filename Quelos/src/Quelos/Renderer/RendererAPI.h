#pragma once

namespace Quelos {
	enum class RendererAPI {
		None = 0,
		OpenGL = 1,
		Vulkan = 2,
		Direct3D11 = 3,
		Direct3D12 = 4,
		Metal = 5
	};
}

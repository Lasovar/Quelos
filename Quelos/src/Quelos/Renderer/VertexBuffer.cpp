#include "qspch.h"
#include "VertexBuffer.h"

#include "Quelos/Platform/bgfx/bgfxVertexBuffer.h"

namespace Quelos {

	Ref<VertexBuffer> VertexBuffer::Create(const std::vector<PosColorVertex>& vertices) {
		return CreateRef<bgfxVertexBuffer>(vertices);
	}
}


#pragma once

namespace Quelos {
	struct PosColorVertex {
		float x;
		float y;
		float z;
		uint32_t abgr;
	};

	class VertexBuffer : public RefCounted {
	public:
		virtual void Bind(uint32_t stream) const = 0;

	public:
		static Ref<VertexBuffer> Create(const std::vector<PosColorVertex>& vertices);
	};
}


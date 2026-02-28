#pragma once

namespace Quelos {
	class IndexBuffer {
	public:
		virtual ~IndexBuffer() = default;

		virtual void Bind() const = 0;
	public:
		static Ref<IndexBuffer> Create(const std::vector<uint16_t>& indices);
	};
}

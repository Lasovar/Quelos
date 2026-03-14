#pragma once

#include "Quelos/Utility/SlotMap.h"

namespace Quelos {
	class IndexBuffer;

	struct IndexBufferHandle : Handle<IndexBuffer> {
		IndexBufferHandle() = default;

		IndexBufferHandle(const Handle handle) {
			Value = handle.Value;
		}

		void Bind() const;
	};
}

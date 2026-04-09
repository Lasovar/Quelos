#pragma once

#include "Quelos/Utility/SlotMap.h"

namespace Quelos {
	class QS_API IndexBuffer;

	struct QS_API IndexBufferHandle : Handle<IndexBuffer> {
		IndexBufferHandle() = default;

		IndexBufferHandle(const Handle handle) {
			Value = handle.Value;
		}

		void Bind() const;
	};
}

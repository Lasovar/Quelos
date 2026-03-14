#pragma once
#include "Quelos/Utility/SlotMap.h"

namespace Quelos {
	struct PosColorVertex {
		float x;
		float y;
		float z;
		uint32_t abgr;

		template <typename TArchive>
		static void Serialize(TArchive& archive, PosColorVertex& data) {
			archive.Value(data.x);
			archive.Value(data.y);
			archive.Value(data.z);
			archive.Value(data.abgr);
		}
	};

	class VertexBuffer;

	struct VertexBufferHandle : Handle<VertexBuffer> {
		VertexBufferHandle() = default;
		VertexBufferHandle(const Handle handle) {
			Value = handle.Value;
		}

		void Bind(uint32_t stream = 0) const;
	};
}

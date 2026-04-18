#pragma once
#include "Quelos/Utility/SlotMap.h"

namespace Quelos {
	struct QS_API PosColorVertex {
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

	struct QS_API Vertex {
		glm::vec3 Position;
		glm::vec3 Normal;
		glm::vec3 Tangent;
		glm::vec2 UV;
	};

	class QS_API VertexBuffer;

	struct QS_API VertexBufferHandle : Handle<VertexBuffer> {
		VertexBufferHandle() = default;
		VertexBufferHandle(const Handle handle) {
			Value = handle.Value;
		}

		void Bind(uint32_t stream = 0) const;

		operator uint64_t() const { return Value; }
	};
}

#include <qspch.h>
#include "Mesh.h"

namespace Quelos {
	Mesh::Mesh(std::vector<PosColorVertex> verticies, std::vector<uint16_t> indicies) {
		m_VertexBuffer = CreateRef<VertexBuffer>(verticies);
		m_IndexBuffer = CreateRef<IndexBuffer>(indicies);
	}

}

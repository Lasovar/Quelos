#include <qspch.h>
#include "Mesh.h"

namespace Quelos {
	Mesh::Mesh(std::vector<glm::vec3> verticies, std::vector<uint16_t> indicies) {
		m_VertexBuffer = Ref<VertexBuffer>::Create(verticies.data(), verticies.size());
		m_IndexBuffer = Ref<IndexBuffer>::Create(indicies.data(), indicies.size());
	}

}

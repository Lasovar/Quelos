#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Quelos {
	class Material;
	class IndexBuffer;
	class VertexBuffer;

	struct TransformComponent {
		glm::vec3 Position;
		glm::quat Rotation;
		glm::vec3 Scale;
	};

	struct CameraComponent {
		float FOV;
		float Near;
		float Far;
	};

	struct MeshComponent {
		Ref<VertexBuffer> VertexBuffer;
		Ref<IndexBuffer> IndexBuffer;
		Ref<Material> Material;
	};
}


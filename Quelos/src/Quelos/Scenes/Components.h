#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Quelos {
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

	};
}


#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Quelos/AssetManager/Assets/Mesh.h"

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
		Ref<Mesh> MeshData;
		Ref<Material> MaterialData; // MinGW complaining about using 'Material' here
	};
}

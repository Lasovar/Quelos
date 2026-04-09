#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include "Quelos/Scenes/Components.h"

namespace Quelos::Math {
    QS_API glm::mat4 OrthographicMatrix(float left, float right, float bottom, float top, float zNear, float zFar);
    QS_API glm::mat4 PerspectiveMatrix(float fov, float aspectRatio, float nearClip, float farClip);
    QS_API glm::mat4 ViewMatrix(const glm::mat4& world);
    QS_API glm::mat4 ViewMatrix(const glm::quat& rotation, const glm::vec3& position);
    QS_API glm::mat4 SRTMatrix(const LocalTransform& transform);
    QS_API glm::mat4 SRTMatrix(const glm::vec3& scale, const glm::vec3& eulerAngles, const glm::vec3& position);
    QS_API glm::mat4 SRTMatrix(const glm::vec3& scale, const glm::quat& rotation, const glm::vec3& position);
}

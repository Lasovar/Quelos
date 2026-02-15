#pragma once

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Quelos::Math {
    glm::mat4 OrthographicMatrix(float left, float right, float bottom, float top, float zNear, float zFar);
    glm::mat4 PerspectiveMatrix(float fov, float aspectRation, float nearClip, float farClip);
    glm::mat4 ViewMatrix(const glm::quat& rotation, const glm::vec3& position);
    glm::mat4 SRTMatrix(const glm::vec3& scale, const glm::vec3& eulerAngles, const glm::vec3& position);
    glm::mat4 SRTMatrix(const glm::vec3& scale, const glm::quat& rotation, const glm::vec3& position);
}

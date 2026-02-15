#pragma once

#include "glm/glm.hpp"

namespace Quelos::Math {
    glm::mat4 ViewMatrix(const glm::quat& rotation, const glm::vec3& position);
    glm::mat4 SRTMatrix(const glm::vec3& scale, const glm::vec3& eulerAngles, const glm::vec3& position);
    glm::mat4 SRTMatrix(const glm::vec3& scale, const glm::quat& rotation, const glm::vec3& position);
}

module;
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

export module Quelos.Math;

export namespace Quelos::Math {
    glm::mat4 OrthographicMatrix(float left, float right, float bottom, float top, float zNear, float zFar);
    glm::mat4 PerspectiveMatrix(float fov, float aspectRatio, float nearClip, float farClip);
    glm::mat4 ViewMatrix(const glm::quat& rotation, const glm::vec3& position);
    glm::mat4 SRTMatrix(const glm::vec3& scale, const glm::vec3& eulerAngles, const glm::vec3& position);
    glm::mat4 SRTMatrix(const glm::vec3& scale, const glm::quat& rotation, const glm::vec3& position);
}

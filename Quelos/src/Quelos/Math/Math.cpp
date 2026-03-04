#include "Math.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/quaternion.hpp"

#include "bgfx/bgfx.h"

namespace Quelos::Math {
    glm::mat4 OrthographicMatrix(
        const float left,
        const float right,
        const float bottom,
        const float top,
        const float zNear,
        const float zFar
    ) {
        const glm::mat4 projection = bgfx::getCaps()->homogeneousDepth
                                         ? glm::orthoLH_NO(
                                             left,
                                             right,
                                             bottom,
                                             top,
                                             zNear,
                                             zFar
                                         )
                                         : glm::orthoLH_ZO(
                                             left,
                                             right,
                                             bottom,
                                             top,
                                             zNear,
                                             zFar
                                         );

        return projection;
    }

    glm::mat4 PerspectiveMatrix(const float fov, const float aspectRatio, const float nearClip, const float farClip) {
        const glm::mat4 projection = bgfx::getCaps()->homogeneousDepth
                                         ? glm::perspectiveLH_NO(
                                             fov,
                                             aspectRatio,
                                             nearClip,
                                             farClip
                                         )
                                         : glm::perspectiveLH_ZO(
                                             fov,
                                             aspectRatio,
                                             nearClip,
                                             farClip
                                         );

        return projection;
    }

    glm::mat4 ViewMatrix(const glm::quat& rotation, const glm::vec3& position) {
        return glm::inverse(
            glm::translate(glm::mat4(1.0f), position) *
            glm::mat4_cast(rotation)
        );
    }

    glm::mat4 SRTMatrix(const glm::vec3& scale, const glm::vec3& eulerAngles, const glm::vec3& position) {
        return SRTMatrix(scale, glm::quat(eulerAngles), position);
    }

    glm::mat4 SRTMatrix(const glm::vec3& scale, const glm::quat& rotation, const glm::vec3& position) {
        return glm::translate(glm::mat4(1.0f), position) *
            glm::mat4_cast(glm::normalize(rotation)) *
            glm::scale(glm::mat4(1.0f), scale);
    }
}

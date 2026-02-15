#include "qspch.h"
#include "Math.h"

#include "bgfx/bgfx.h"
#include "bx/math.h"

namespace Quelos::Math {
    glm::mat4 OrthographicMatrix(
        const float left,
        const float right,
        const float bottom,
        const float top,
        const float zNear,
        const float zFar
    ) {
        glm::mat4 projection;
        if (bgfx::getCaps()->homogeneousDepth) {
            projection = glm::orthoLH_NO(
                left,
                right,
                bottom,
                top,
                zNear,
                zFar
            );
        } else {
            projection = glm::orthoLH_ZO(
                left,
                right,
                bottom,
                top,
                zNear,
                zFar
            );
        }

        return projection;
    }

    glm::mat4 PerspectiveMatrix(const float fov, const float aspectRatio, const float nearClip, const float farClip) {
        glm::mat4 projection;
        if (bgfx::getCaps()->homogeneousDepth) {
            // OpenGL style: Z = -1..1
            projection = glm::perspectiveLH_NO(
                fov,
                aspectRatio,
                nearClip,
                farClip
            );
        }
        else {
            // DX/Vulkan/Metal: Z = 0..1
            projection = glm::perspectiveLH_ZO(
                fov,
                aspectRatio,
                nearClip,
                farClip
            );
        }

        return projection;
    }

    glm::mat4 ViewMatrix(const glm::quat& rotation, const glm::vec3& position) {
        return glm::inverse(
            glm::translate(glm::mat4(1.0f), position) *
            glm::mat4_cast(rotation)
        );
    }

    glm::mat4 SRTMatrix(const glm::vec3& scale, const glm::vec3& eulerAngles, const glm::vec3& position) {
        glm::mat4 result;
        bx::mtxSRT(
            glm::value_ptr(result),
            scale.x, scale.y, scale.z,
            eulerAngles.x, eulerAngles.y, eulerAngles.z,
            position.x, position.y, position.z
        );

        return result;
    }

    glm::mat4 SRTMatrix(const glm::vec3& scale, const glm::quat& rotation, const glm::vec3& position) {
        return glm::translate(glm::mat4(1.0f), position) *
            glm::mat4_cast(glm::normalize(rotation)) *
            glm::scale(glm::mat4(1.0f), scale);
    }
}

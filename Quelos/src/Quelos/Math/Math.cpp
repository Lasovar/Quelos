#include "qspch.h"
#include "Math.h"

#include "bx/math.h"
#include "glm/gtc/type_ptr.hpp"

namespace Quelos::Math {
    glm::mat4 ViewMatrix(const glm::quat& rotation, const glm::vec3& position) {
        const bx::Quaternion quat(
            rotation.x,
            rotation.y,
            rotation.z,
            rotation.w
        );

        glm::mat4 rot;
        bx::mtxFromQuaternion(glm::value_ptr(rot), quat);

        glm::mat4 translation;
        bx::mtxTranslate(glm::value_ptr(translation), position.x, position.y, position.z);

        glm::mat4 world;
        bx::mtxMul(glm::value_ptr(world), glm::value_ptr(translation), glm::value_ptr(rot));

        glm::mat4 result;
        bx::mtxInverse(glm::value_ptr(result), glm::value_ptr(world));

        return result;
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
        // rotation
        const glm::quat normalizedRot = glm::normalize(rotation);;
        const bx::Quaternion quat(normalizedRot.x, normalizedRot.y, normalizedRot.z, normalizedRot.w);

        glm::mat4 result;
        bx::mtxFromQuaternion(glm::value_ptr(result), quat);

        // scale
        result[0] *= scale.x;
        result[1] *= scale.y;
        result[2] *= scale.z;

        // translation
        result[3][0] = position.x;
        result[3][1] = position.y;
        result[3][2] = position.z;

        return result;
    }
}

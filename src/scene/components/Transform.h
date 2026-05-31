#pragma once

#define GLM_ENABLE_EXPERIMENTAL

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>


namespace Lengine {
    struct TransformComponent
    {
        glm::vec3 localPosition{ 0.0f };
        glm::quat localRotation = glm::quat(1, 0, 0, 0); // radians
        glm::vec3 localScale{ 1.0f };

        // cached
        glm::mat4 localMatrix{ 1.0f };
        glm::mat4 worldMatrix{ 1.0f };

        bool localDirty = true;
        bool worldDirty = true;

        glm::vec3 GetWorldPosition() const
        {
            return glm::vec3(worldMatrix[3]);
        }

        glm::vec3 GetWorldScale()
        {
            return {
                glm::length(glm::vec3(worldMatrix[0])),
                glm::length(glm::vec3(worldMatrix[1])),
                glm::length(glm::vec3(worldMatrix[2]))
            };
        }

        glm::quat GetWorldRotation() const
        {
            glm::mat3 rotMatrix;

            rotMatrix[0] = glm::vec3(worldMatrix[0]);
            rotMatrix[1] = glm::vec3(worldMatrix[1]);
            rotMatrix[2] = glm::vec3(worldMatrix[2]);

            // remove scaling
            rotMatrix[0] = glm::normalize(rotMatrix[0]);
            rotMatrix[1] = glm::normalize(rotMatrix[1]);
            rotMatrix[2] = glm::normalize(rotMatrix[2]);

            return glm::quat_cast(rotMatrix);
        }

        glm::mat4 getViewMatrix()
        {
            glm::vec3 position = GetWorldPosition();
            glm::quat rotation = GetWorldRotation();

            glm::mat4 rot = glm::toMat4(glm::quat(rotation));
            glm::mat4 trans = glm::translate(glm::mat4(1.0f), position);

            glm::mat4 world = trans * rot;

            return glm::inverse(world);
        }

        void SetPosition(const glm::vec3& pos)
        {
            localPosition = pos;
            localDirty = true;
            worldDirty = true;
        }

        void Translate(const glm::vec3& delta)
        {
            localPosition += delta;
            localDirty = true;
            worldDirty = true;
        }

        void SetRotation(const glm::quat& rot)
        {
            localRotation = rot;
            localDirty = true;
            worldDirty = true;
        }

        void SetScale(const glm::vec3& scale)
        {
            localScale = scale;
            localDirty = true;
            worldDirty = true;
        }


    };

}




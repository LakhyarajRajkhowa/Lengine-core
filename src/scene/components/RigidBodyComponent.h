#pragma once

#include <PxPhysicsAPI.h>

namespace Lengine {

    struct RigidbodyComponent
    {

        float mass = 1.0f;

        bool useGravity = true;
        bool isKinematic = false;


        glm::vec3 velocity{ 0.0f };
        glm::vec3 acceleration{ 0.0f };

        glm::vec3 forces{ 0.0f };

        float linearDamping = 0.98f;

        bool dirty = true;
    };

}

#pragma once

#include <glm/glm.hpp>

namespace Lengine {

    struct MovementComponent
    {
        glm::vec2 moveInput{ 0.0f };

        bool jumpRequested = false;
        bool sprinting = false;

        float walkSpeed = 5.0f;
        float sprintMultiplier = 2.0f;
        float jumpForce = 8.0f;
    };
}
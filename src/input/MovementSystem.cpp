// MovementSystem.cpp

#include "MovementSystem.h"

namespace Lengine {

    void MovementSystem::Update(float deltaTime)
    {
        auto& scene = sceneManager.GetRuntimeScene();
        auto& registry = scene->GetRegistry();

        for (const Entity& entity :
            registry.GetStorageEntities<MovementComponent>())
        {
            auto& movement =
                registry.movements.Get(entity);

            // Optional controller
            if (registry.controllers.Has(entity))
            {
                auto& controller =
                    registry.controllers.Get(entity);

                ApplyControllerInput(
                    entity,
                    controller,
                    movement);
            }

            // Physics movement
            if (registry.rigidBodies.Has(entity))
            {
                auto& rigidbody =
                    registry.rigidBodies.Get(entity);

                ApplyMovementPhysics(
                    entity,
                    movement,
                    rigidbody);
            }

            // Direct transform movement
            else if (registry.transforms.Has(entity))
            {
                auto& transform =
                    registry.transforms.Get(entity);

                ApplyTransformMovement(
                    entity,
                    movement,
                    transform,
                    deltaTime);
            }
        }
    }

    void MovementSystem::ApplyControllerInput(
        Entity entity,
        ControllerComponent& controller,
        MovementComponent& movement)
    {
        movement.moveInput =
        {
            controller.moveX,
            controller.moveY
        };

        movement.jumpRequested =
            controller.jumpPressed;

        movement.sprinting =
            controller.sprintHeld;
    }

    void MovementSystem::ApplyTransformMovement(
        Entity entity,
        MovementComponent& movement,
        TransformComponent& transform,
        float deltaTime)
    {
        // -------- Speed --------

        float speed = movement.walkSpeed;

        if (movement.sprinting)
            speed *= movement.sprintMultiplier;

        // -------- Movement Direction --------

        glm::vec3 moveDirection(0.0f);

        moveDirection.x =
            movement.moveInput.x;

        moveDirection.z =
            movement.moveInput.y;

        // Normalize
        if (glm::length(moveDirection) > 0.0f)
        {
            moveDirection =
                glm::normalize(moveDirection);
        }

        // -------- Apply Movement --------

        transform.localPosition +=
            moveDirection *
            speed *
            deltaTime;

        // Mark transform dirty
        transform.localDirty = true;
        transform.worldDirty = true;


    }

    void MovementSystem::ApplyMovementPhysics(
        Entity entity,
        MovementComponent& movement,
        RigidbodyComponent& rigidbody)
    {
        if (rigidbody.isKinematic)
            return;

        // -------- Speed --------

        float speed = movement.walkSpeed;

        if (movement.sprinting)
            speed *= movement.sprintMultiplier;

        // -------- Desired Velocity --------

        glm::vec3 desiredVelocity(0.0f);

        desiredVelocity.x =
            movement.moveInput.x * speed;

        desiredVelocity.z =
            movement.moveInput.y * speed;

        // -------- Apply Horizontal Movement --------
        // Arcade-style movement

        rigidbody.velocity.x =
            desiredVelocity.x;

        rigidbody.velocity.z =
            desiredVelocity.z;

        // -------- Jump --------

        if (movement.jumpRequested)
        {
            rigidbody.velocity.y =
                movement.jumpForce;
        }

    }  

}
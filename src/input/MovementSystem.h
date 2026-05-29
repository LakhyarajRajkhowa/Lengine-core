// MovementSystem.h

#pragma once

#include "scene/SceneManager.h"

namespace Lengine {

    class MovementSystem
    {
    public:

        MovementSystem(SceneManager& sceneManager)
            :
            sceneManager(sceneManager)
        {}

        ~MovementSystem() = default;

        void Update(float deltaTime);

    private:

        void ApplyControllerInput(
            Entity entity,
            ControllerComponent& controller,
            MovementComponent& movement);

        void ApplyTransformMovement(
            Entity entity,
            MovementComponent& movement,
            TransformComponent& transform,
            float deltaTime);

        void ApplyMovementPhysics(
            Entity entity,
            MovementComponent& movement,
            RigidbodyComponent& rigidbody);

    private:

        SceneManager& sceneManager;
    };

}
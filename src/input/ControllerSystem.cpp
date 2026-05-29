#include "ControllerSystem.h"

#include <iostream>

namespace Lengine {

    void ControllerSystem::Update(float deltaTime)
    {
        auto& scene = sceneManager.GetRuntimeScene();
        auto& registry = scene->GetRegistry();

        for (const Entity& entity :
            registry.GetStorageEntities<ControllerComponent>())
        {
            auto& controller =
                registry.GetComponent<ControllerComponent>(entity);

            // Clear previous frame state
            ResetControllerState(controller);

            if (!controller.active)
                continue;

            switch (controller.type)
            {
            case ControllerType::Player:
            {
                UpdatePlayerController(controller);
                break;
            }

            case ControllerType::AI:
            {
                // TODO:
                // AI controller logic
                break;
            }

            default:
                break;
            }
        }
    }

    void ControllerSystem::ResetControllerState(
        ControllerComponent& controller)
    {
        // Movement axes
        controller.moveX = 0.0f;
        controller.moveY = 0.0f;

        // Mouse look
        controller.lookX = 0.0f;
        controller.lookY = 0.0f;

        // Buttons
        controller.jumpPressed = false;
        controller.attackPressed = false;
        controller.interactPressed = false;
        controller.pausePressed = false;

        controller.sprintHeld = false;
    }

    void ControllerSystem::UpdatePlayerController(
        ControllerComponent& controller)
    {
        const GameKeys& keys = controller.keybinds;

        // -------- Movement --------

        if (inputManager.isKeyDown(keys.moveForward)) 
            controller.moveY += 1.0f;
        
        if (inputManager.isKeyDown(keys.moveBackward))
            controller.moveY -= 1.0f;

        if (inputManager.isKeyDown(keys.moveRight))
            controller.moveX += 1.0f;

        if (inputManager.isKeyDown(keys.moveLeft))
            controller.moveX -= 1.0f;

        // -------- Actions --------

        controller.jumpPressed =
            inputManager.isKeyPressed(keys.jump);



        controller.interactPressed =
            inputManager.isKeyPressed(keys.interact);

        controller.pausePressed =
            inputManager.isKeyPressed(keys.pause);

        controller.sprintHeld =
            inputManager.isKeyDown(keys.sprint);

        // -------- Mouse --------

        controller.attackPressed =
            inputManager.isMouseButtonPressed(
                keys.attack);


        glm::vec2 mouseDelta =
            inputManager.getMouseDelta();

        controller.lookX = mouseDelta.x;
        controller.lookY = mouseDelta.y;

        // -------- Normalize Movement --------
        // Prevent diagonal speed boost

        glm::vec2 move(
            controller.moveX,
            controller.moveY);

        if (glm::length(move) > 1.0f)
        {
            move = glm::normalize(move);

            controller.moveX = move.x;
            controller.moveY = move.y;
        }
    }

}
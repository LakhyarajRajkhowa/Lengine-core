#pragma once

#include "InputManager.h"
#include "scene/SceneManager.h"

#include <SDL2/SDL.h>
#include <cmath>

namespace Lengine {

    class PlayerInputHandler : public IInputHandler
    {
    public:

        explicit PlayerInputHandler(SceneManager& sceneMgr)
            : sceneManager(sceneMgr)
        {}

        void onUpdate(float /*dt*/, InputManager& input) override
        {
            auto controllers = getActiveControllers();

            for (ControllerComponent* controller : controllers)
            {
                if (input.isKeyDown(controller->keybinds.moveForward))
                    controller->moveY += 1.0f;


                if (!controller)
                    return;

                float x = 0.0f;
                float z = 0.0f;

                // ------------------------------------------------------------
                // Movement
                // ------------------------------------------------------------

                if (input.isKeyDown(controller->keybinds.moveRight))
                    x += 1.0f;

                if (input.isKeyDown(controller->keybinds.moveLeft))
                    x -= 1.0f;

                if (input.isKeyDown(controller->keybinds.moveForward))
                    z += 1.0f;

                if (input.isKeyDown(controller->keybinds.moveBackward))
                    z -= 1.0f;

                // Normalize diagonal movement
                if (x != 0.0f && z != 0.0f)
                {
                    const float inv = 1.0f / std::sqrt(2.0f);
                    x *= inv;
                    z *= inv;
                }

                controller->moveX = x;
                controller->moveY = z;

                // ------------------------------------------------------------
                // Held states
                // ------------------------------------------------------------

                controller->sprintHeld =
                    input.isKeyDown(controller->keybinds.sprint);

                // ------------------------------------------------------------
                // Mouse look
                // ------------------------------------------------------------

                glm::vec2 mouseDelta = input.getMouseDelta();

                controller->lookX = mouseDelta.x;
                controller->lookY = mouseDelta.y;
            }
        }


        void onEvent(const SDL_Event& event, InputManager& /*input*/) override
        {
            auto controllers = getActiveControllers();

            for (ControllerComponent* controller : controllers)
            {

                if (!controller)
                    return;

                switch (event.type)
                {
                case SDL_KEYDOWN:

                    // Ignore held-repeat spam
                    if (event.key.repeat)
                        return;

                    switch (event.key.keysym.sym)
                    {
                    case SDLK_TAB:
                        // Example:
                        // Character switching system could go here
                        // switchCharacter();
                        break;

                    default:
                        break;
                    }

                    // --------------------------------------------------------
                    // Action bindings
                    // --------------------------------------------------------

                    if (event.key.keysym.sym == controller->keybinds.jump)
                    {
                        controller->jumpPressed = true;
                    }

                    if (event.key.keysym.sym == controller->keybinds.interact)
                    {
                        controller->interactPressed = true;
                    }

                    if (event.key.keysym.sym == controller->keybinds.pause)
                    {
                        controller->pausePressed = true;
                    }

                    break;

                case SDL_MOUSEBUTTONDOWN:

                    if (event.button.button ==
                        controller->keybinds.attack)
                    {
                        controller->attackPressed = true;
                    }

                    break;

                default:
                    break;
                }
            }
        }

    private:


        std::vector<ControllerComponent*> getActiveControllers()
        {
            std::vector<ControllerComponent*> controllers;

            auto& scene = sceneManager.GetRuntimeScene();
            auto& registry = scene->GetRegistry();

            for (const Entity& entity :
                registry.GetStorageEntities<ControllerComponent>())
            {
                auto& controller =
                    registry.GetComponent<ControllerComponent>(entity);

                if (controller.active)
                    controllers.push_back(&controller);
            }

            return controllers;
        }

    private:

        SceneManager& sceneManager;
    };

} // namespace Lengine
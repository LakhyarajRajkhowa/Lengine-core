#pragma once

#include "scene/SceneManager.h"
#include "input/InputManager.h"

namespace Lengine {

    class ControllerSystem
    {
    public:

        ControllerSystem(SceneManager& scene,
            InputManager& inputManager)
            :
            sceneManager(scene),
            inputManager(inputManager)
        {}

        ~ControllerSystem() = default;

        void Update(float deltaTime);

    private:

        void UpdatePlayerController(
            ControllerComponent& controller);

        void ResetControllerState(
            ControllerComponent& controller);

    private:

        SceneManager& sceneManager;
        InputManager& inputManager;
    };

}
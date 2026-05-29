#pragma once

#include "input/KeyBindings.h"

namespace Lengine {

    enum class ControllerType
    {
        None = 0,
        Player,
        AI
    };

    struct ControllerComponent
    {


        bool active = false;

        ControllerType type;
        GameKeys keybinds;

        // Movement axes
        float moveX = 0.0f;
        float moveY = 0.0f;

        // Mouse look delta
        float lookX = 0.0f;
        float lookY = 0.0f;

        // Held buttons
        bool sprintHeld = false;


        bool jumpPressed = false;
        bool attackPressed = false;
        bool interactPressed = false;
        bool pausePressed = false;
    };


}
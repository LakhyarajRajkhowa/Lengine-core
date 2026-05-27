#pragma once

#include <glm/glm.hpp>

namespace Lengine {

    // -----------------------------------------------------------------------
    //  Game-level event types (runtime only, never fire in editor mode)
    // -----------------------------------------------------------------------
    enum class GameEventType
    {
        // Input-derived movement intents forwarded to ControllerComponent
        MoveAxis,       // payload: MoveAxisData  (WASD / stick)
        LookAxis,       // payload: LookAxisData  (mouse delta / right-stick)
        Jump,           // payload: none          (spacebar pressed)
        Sprint,         // payload: SprintData    (shift held/released)

        // Gameplay lifecycle
        Pause,          // game paused / unpaused
        Interact,       // 'E' key etc. — near-object interaction intent
        Attack,         // left mouse / fire button

        // Engine/scene lifecycle (fired by EngineCore)
        PlayModeEntered,
        PlayModeExited,
    };

    // -----------------------------------------------------------------------
    //  Payloads  (keep them trivially copyable — they live inside GameEvent)
    // -----------------------------------------------------------------------
    struct MoveAxisData
    {
        float x = 0.0f;   // right/left  (-1 .. +1)
        float z = 0.0f;   // forward/back (-1 .. +1)
    };

    struct LookAxisData
    {
        float deltaX = 0.0f;
        float deltaY = 0.0f;
    };

    struct SprintData
    {
        bool active = false;
    };

    // -----------------------------------------------------------------------
    //  The event envelope
    // -----------------------------------------------------------------------
    struct GameEvent
    {
        GameEventType type;

        union Payload
        {
            MoveAxisData  move;
            LookAxisData  look;
            SprintData    sprint;
            Payload() {}   // zero-init
        } payload;

        // Convenience factories --------------------------------------------------

        static GameEvent makeMoveAxis(float x, float z)
        {
            GameEvent e;
            e.type = GameEventType::MoveAxis;
            e.payload.move = { x, z };
            return e;
        }

        static GameEvent makeLookAxis(float dx, float dy)
        {
            GameEvent e;
            e.type = GameEventType::LookAxis;
            e.payload.look = { dx, dy };
            return e;
        }

        static GameEvent makeSimple(GameEventType t)
        {
            GameEvent e;
            e.type = t;
            return e;
        }

        static GameEvent makeSprint(bool active)
        {
            GameEvent e;
            e.type = GameEventType::Sprint;
            e.payload.sprint = { active };
            return e;
        }
    };

} // namespace Lengine
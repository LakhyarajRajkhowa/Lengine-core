#pragma once

// ============================================================================
//  PlayerInputHandler
//
//  Active only in InputContext::Game (play mode).
//  Reads raw key/mouse state from InputManager and translates it into
//  strongly-typed GameEvents dispatched through GameEventSystem.
//
//  Does NOT touch ControllerComponent directly — that's GameController's job.
//  This keeps input translation decoupled from physics/movement logic.
// ============================================================================

#include "InputRouter.h"
#include "GameEventSystem.h"
#include "GameEvent.h"


#include <SDL2/SDL.h>

namespace Lengine {

    // -----------------------------------------------------------------------
    //  Game key bindings (separate from EditorKeys so they never conflict)
    // -----------------------------------------------------------------------
    namespace GameKeys
    {
        inline constexpr SDL_Keycode MoveForward = SDLK_w;
        inline constexpr SDL_Keycode MoveBackward = SDLK_s;
        inline constexpr SDL_Keycode MoveLeft = SDLK_a;
        inline constexpr SDL_Keycode MoveRight = SDLK_d;
        inline constexpr SDL_Keycode Jump = SDLK_SPACE;
        inline constexpr SDL_Keycode Sprint = SDLK_LSHIFT;
        inline constexpr SDL_Keycode Interact = SDLK_e;
        inline constexpr SDL_Keycode Pause = SDLK_ESCAPE;
    }

    // -----------------------------------------------------------------------
    //  Handler
    // -----------------------------------------------------------------------
    class PlayerInputHandler : public IInputHandler
    {
    public:
        explicit PlayerInputHandler(GameEventSystem& gameEvents)
            : m_gameEvents(gameEvents)
        {}

        // ------------------------------------------------------------------
        //  Continuous input (held keys, mouse motion) — called each frame
        // ------------------------------------------------------------------
        void onUpdate(float /*dt*/, InputManager& input) override
        {
            // --- Movement axes: build from held keys ---
            float x = 0.0f, z = 0.0f;

            if (input.isKeyDown(GameKeys::MoveRight))   x += 1.0f;
            if (input.isKeyDown(GameKeys::MoveLeft))    x -= 1.0f;
            if (input.isKeyDown(GameKeys::MoveForward)) z += 1.0f;
            if (input.isKeyDown(GameKeys::MoveBackward))z -= 1.0f;

            // Normalise diagonal movement
            if (x != 0.0f && z != 0.0f)
            {
                const float inv = 1.0f / std::sqrt(2.0f);
                x *= inv;
                z *= inv;
            }

            // Always dispatch move axis (even 0,0) so GameController can
            // clear its desiredDirection when no key is held.
            m_gameEvents.dispatch(GameEvent::makeMoveAxis(x, z));

            // --- Mouse look ---
            // We use SDL relative mouse mode for look, so delta is in event.
            // But sprint is a held state so poll it here.
            if (input.isKeyDown(GameKeys::Sprint))
                m_gameEvents.dispatch(GameEvent::makeSprint(true));
        }

        // ------------------------------------------------------------------
        //  Edge-triggered events (key press/release, mouse click) — per event
        // ------------------------------------------------------------------
        void onEvent(const SDL_Event& event, InputManager& /*input*/) override
        {
            switch (event.type)
            {
            case SDL_KEYDOWN:
                if (event.key.repeat) break; // ignore key-repeat for one-shot actions

                switch (event.key.keysym.sym)
                {
                case GameKeys::Jump:
                    m_gameEvents.dispatch(GameEvent::makeSimple(GameEventType::Jump));
                    break;

                case GameKeys::Interact:
                    m_gameEvents.dispatch(GameEvent::makeSimple(GameEventType::Interact));
                    break;

                case GameKeys::Pause:
                    m_gameEvents.dispatch(GameEvent::makeSimple(GameEventType::Pause));
                    break;

                default: break;
                }
                break;

            case SDL_KEYUP:
                switch (event.key.keysym.sym)
                {
                case GameKeys::Sprint:
                    m_gameEvents.dispatch(GameEvent::makeSprint(false));
                    break;
                default: break;
                }
                break;

            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_LEFT)
                    m_gameEvents.dispatch(GameEvent::makeSimple(GameEventType::Attack));
                break;

            case SDL_MOUSEMOTION:
                // Mouse look delta — only meaningful when cursor is locked
                m_gameEvents.dispatch(
                    GameEvent::makeLookAxis(
                        static_cast<float>(event.motion.xrel),
                        static_cast<float>(event.motion.yrel)
                    )
                );
                break;

            default: break;
            }
        }

    private:
        GameEventSystem& m_gameEvents;
    };

} // namespace Lengine
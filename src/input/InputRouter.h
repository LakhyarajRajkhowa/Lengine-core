#pragma once

// ============================================================================
//  InputRouter
//
//  Sits between raw SDL events and the rest of the engine.
//  Decides WHO gets keyboard/mouse input at any given frame based on
//  the active InputContext (UI > EditorCamera > Game).
//
//  Flow:
//
//   SDL_Event
//       │
//       ▼
//   InputRouter::route(event)
//       │
//       ├─[always]──► InputManager::processEvent()    (raw key/mouse state)
//       │   
//       ├─[ctx==UI]──► ImGui capture
//       │
//       ├─[ctx==EditorCamera]──► EditorCameraHandler (mouse look + WASD fly)
//       │
//       └─[ctx==Game]──► PlayerInputHandler ──► GameEventSystem
//                                                      │
//                                                      ▼
//                                              GameController / ControllerComponent
//
// ============================================================================

#include <SDL2/SDL.h>
#include <imgui.h>
#include <functional>

#include "InputManager.h"     
#include "InputContext.h"



namespace Lengine {

    class IInputHandler
    {
    public:
        virtual ~IInputHandler() = default;

        virtual void onUpdate(float dt, InputManager& input) {}
        virtual void onEvent(const SDL_Event& event, InputManager& input) {}
    };

    class InputRouter
    {
    public:

        explicit InputRouter(InputManager& inputManager)
            : m_input(inputManager)
        {}


        void setContext(InputContext ctx)
        {
            m_context = ctx;
        }

        InputContext getContext() const { return m_context; }

        bool isContext(InputContext ctx) const { return m_context == ctx; }
        void setGameHandler(IInputHandler* handler) { m_gameHandler = handler; }
        void setEditorHandler(IInputHandler* handler) { m_editorHandler = handler; }
        void setUIHandler(IInputHandler* handler) { m_uiHandler = handler; }


        void update(float dt)
        {

            if (m_context == InputContext::Game && m_gameHandler)
                m_gameHandler->onUpdate(dt, m_input);

            else if (m_context == InputContext::EditorCamera && m_editorHandler)
                m_editorHandler->onUpdate(dt, m_input);
        }

        void routeEvent(const SDL_Event& event)
        {
            if (event.type == SDL_QUIT ||
                event.type == SDL_WINDOWEVENT)
            {
                return;
            }


            if (m_context == InputContext::Game && m_gameHandler) {
                m_gameHandler->onEvent(event, m_input);
            }

            else if (m_context == InputContext::EditorCamera && m_editorHandler) {
                m_editorHandler->onEvent(event, m_input);
            }

            else if (m_context == InputContext::UI && m_uiHandler) {
                m_uiHandler->onEvent(event, m_input);

            }
        }


    private:
        InputManager& m_input;
        InputContext     m_context = InputContext::UI;

        IInputHandler* m_gameHandler = nullptr;
        IInputHandler* m_editorHandler = nullptr;
        IInputHandler* m_uiHandler = nullptr;

    };

} // namespace Lengine
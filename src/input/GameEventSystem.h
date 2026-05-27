#pragma once

#include "GameEvent.h"

#include <functional>
#include <vector>
#include <unordered_map>

namespace Lengine {

    // -----------------------------------------------------------------------
    //  GameEventSystem
    //
    //  A typed, synchronous dispatcher for in-game events.
    //  Completely independent of SDL_Event — nothing SDL-specific leaks here.
    //
    //  Usage:
    //      gameEventSystem.subscribe(GameEventType::Jump, [](const GameEvent& e) {
    //          player.jump();
    //      });
    //
    //      // later, from PlayerInputHandler:
    //      gameEventSystem.dispatch(GameEvent::makeSimple(GameEventType::Jump));
    //
    //  Thread-safety: single-threaded only (same as the rest of the engine).
    // -----------------------------------------------------------------------
    class GameEventSystem
    {
    public:
        using Callback = std::function<void(const GameEvent&)>;

        // Subscribe to a specific event type
        void subscribe(GameEventType type, Callback callback)
        {
            listeners[type].push_back(std::move(callback));
        }

        // Fire an event synchronously to all subscribers of that type
        void dispatch(const GameEvent& event) const
        {
            auto it = listeners.find(event.type);
            if (it == listeners.end()) return;

            for (const auto& cb : it->second)
                cb(event);
        }

        // Convenience overload
        void dispatch(GameEventType type)
        {
            dispatch(GameEvent::makeSimple(type));
        }

        // Clear all listeners (useful on play-mode exit / scene reload)
        void clearAll()
        {
            listeners.clear();
        }

        void clearType(GameEventType type)
        {
            listeners.erase(type);
        }

    private:
        std::unordered_map<GameEventType,
            std::vector<Callback>> listeners;
    };

} // namespace Lengine
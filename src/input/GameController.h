/*
	!!! CURRENT ARCHITECTURE DO NOT USE THIS !!!
*/


//#pragma once
//
//
//
//// ============================================================================
////  GameController
////
////  Bridges GameEventSystem → ECS components (ControllerComponent,
////  MovementComponent) for every entity with ControllerType::Player.
////
////  Responsibilities:
////    • Subscribe to movement/look/jump GameEvents at construction.
////    • On each event write the intent into the scene's ControllerComponents.
////    • The PhysicsSystem then reads ControllerComponent each frame.
////
////  It never reads SDL_Event or InputManager directly.
////  That isolation is the whole point.
//// ============================================================================
//
//#include "GameEventSystem.h"
//
//#include "scene/Scene.h"
//
//
//namespace Lengine {
//
//    class GameController
//    {
//    public:
//
//        GameController(GameEventSystem& gameEvents, Scene& scene)
//            : m_gameEvents(gameEvents)
//            , scene(scene)
//        {
//            subscribe();
//        }
//
//        // Call this when the runtime scene changes (e.g. scene reload)
//        void bindScene(Scene& scene)
//        {
//            scene = scene;
//        }
//
//        // Called by EngineCore when entering/exiting play mode
//        void onPlayModeEntered()
//        {
//            // Reset all controller intent so no phantom movement on start
//            forEachPlayerController([](ControllerComponent& c, MovementComponent* m) {
//                c.moveX = 0.0f;
//                c.moveY = 0.0f;
//                c.jump = false;
//                if (m) { m->desiredDirection = glm::vec3(0.0f); m->velocity = glm::vec3(0.0f); }
//                });
//        }
//
//        void onPlayModeExited()
//        {
//            onPlayModeEntered(); // same reset
//        }
//
//    private:
//
//        void subscribe()
//        {
//            // --- Movement axes -----------------------------------------------
//            m_gameEvents.subscribe(GameEventType::MoveAxis,
//                [this](const GameEvent& e)
//                {
//                    const auto& data = e.payload.move;
//                    forEachPlayerController([&](ControllerComponent& c, MovementComponent* m) {
//                        c.moveX = data.x;
//                        c.moveY = data.z;
//                        if (m)
//                            m->desiredDirection = glm::vec3(data.x, 0.0f, data.z);
//                        });
//                });
//
//            // --- Look axis (camera / character rotation) ---------------------
//            m_gameEvents.subscribe(GameEventType::LookAxis,
//                [this](const GameEvent& e)
//                {
//                    // Store in ControllerComponent for the physics/character
//                    // system to consume. Actual camera follow is handled by the
//                    // camera component system, not here.
//                    // (extend ControllerComponent with lookDeltaX/Y as needed)
//                    (void)e; // placeholder — extend as your camera system grows
//                });
//
//            // --- Jump ---------------------------------------------------------
//            m_gameEvents.subscribe(GameEventType::Jump,
//                [this](const GameEvent&)
//                {
//                    forEachPlayerController([](ControllerComponent& c, MovementComponent* m) {
//                        if (m && m->grounded)
//                            c.jump = true;
//                        });
//                });
//
//            // --- Sprint -------------------------------------------------------
//            m_gameEvents.subscribe(GameEventType::Sprint,
//                [this](const GameEvent& e)
//                {
//                    const bool sprinting = e.payload.sprint.active;
//                    forEachPlayerController([&](ControllerComponent& c, MovementComponent* m) {
//                        if (m)
//                            m->moveSpeed = sprinting ? m->moveSpeed * 2.0f : m->moveSpeed;
//                        // TODO: store sprint flag in MovementComponent if needed
//                        });
//                });
//
//            // --- Pause --------------------------------------------------------
//            m_gameEvents.subscribe(GameEventType::Pause,
//                [this](const GameEvent&)
//                {
//                    // Broadcast to game-level pause system
//                    // (extend with your pause manager / UI layer)
//                    m_paused = !m_paused;
//                });
//        }
//
//        // Helper: iterate all entities that have ControllerType::Player
//        template<typename Fn>
//        void forEachPlayerController(Fn&& fn)
//        {
//             auto& registry = scene.get().GetRegistry();
//
//             // !!! for now no controller comp added !!!
//
//            //for (const Entity& entity : registry.GetStorageEntities<ControllerComponent>())
//            //{
//            //    ControllerComponent& ctrl = registry.GetComponent<ControllerComponent>(entity);
//
//            //    if (ctrl.type != ControllerType::Player)
//            //        continue;
//
//            //    MovementComponent* movement = nullptr;
//            //    if (registry.HasComponent<MovementComponent>(entity))
//            //        movement = &registry.GetComponent<MovementComponent>(entity);
//
//            //    fn(ctrl, movement);
//            //}
//        }
//
//        GameEventSystem& m_gameEvents;
//        std::reference_wrapper<Scene> scene;
//        bool                         m_paused = false;
//    };
//
//} // namespace Lengine
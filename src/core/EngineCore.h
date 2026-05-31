#pragma once

#include <external/json.hpp>

#include "core/Timer.h"
#include "core/settings.h"
#include "core/EventSystem.h"

#include "graphics/renderer/RenderPipeline.h"
#include "platform/Window.h"

#include "animations/AnimationSystem.h"
#include "transform/TransformSystem.h"
#include "physics/PhysicsSystem.h"
#include "scripting/ScriptSystem.h"

#include "input/InputManager.h"
#include "input/InputContext.h"
#include "input/InputRouter.h"
#include "input/ControllerSystem.h"
#include "input/MovementSystem.h"

#include "utils/fps.h"

namespace Lengine {

    enum class EngineMode
    {
        Editor,
        Play
    };

    class EngineCore {
    public:

        EngineCore();

        void initSystems();
        void run(const EditorMode mode);
        void presentFrame();
        void shutdown();

        void pollEvents();

        void enterPlayMode();
        void exitPlayMode();


        bool& isRunning();

        Window& getWindow();
        InputManager& getInputManager();
        AssetManager& getAssetManager();
        SceneManager& getSceneManager();
        RenderPipeline& getRenderPipeline();
        RenderSettings& getRenderSettings();
        RuntimeStats& getRuntimeStats();
        PhysicsSystem& getPhysicsSystem();
        ScriptSystem& getScriptSystem();

        InputRouter& getInputRouter() { return inputRouter; }

    private:

        void UpdateTimer();
        void updateRuntime(const EditorMode& mode);
        void updateEssentials(const EditorMode& mode);



    private:

        RuntimeStats runtimeStats;
        EngineSettings settings;
        RenderSettings renderSettings;

        Window window;
        InputManager inputManager;
        AssetManager assetManager;
        SceneManager sceneManager;
        RenderPipeline renderPipeline;

        AnimationSystem animationSystem;
        TransformSystem transformSystem;
        PhysicsSystem physicsSystem;
        ControllerSystem controllerSystem;
        MovementSystem movementSystem;
        ScriptSystem scriptSystem;

        InputRouter     inputRouter;      

        bool running = true;

        float deltaTime;

    };

}
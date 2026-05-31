#include "EngineCore.h"

namespace Lengine {

    EngineCore::EngineCore() :

        window(
            settings.windowName,
            settings.windowWidth,
            settings.windowHeight,
            settings.windowMode
        ),

        sceneManager(assetManager, physicsSystem),
        assetManager(settings),
        renderPipeline(assetManager),
        animationSystem(assetManager),
        inputRouter(inputManager),
        controllerSystem(sceneManager, inputManager),
        movementSystem(sceneManager),
        scriptSystem(sceneManager, inputManager)

    {
    }

    void EngineCore::initSystems()
    {
        InitTimer();

        renderSettings.resolution_X = settings.resolution_X;
        renderSettings.resolution_Y = settings.resolution_Y;
     
        std::vector<std::string> scenesTobeLoaded;
        scenesTobeLoaded.push_back("light_test");

        sceneManager.loadScenes(scenesTobeLoaded);

        assetManager.Init();

        renderPipeline.Init();

        physicsSystem.Init();

        scriptSystem.LoadLibrary("GameScripts.dll");

    }

    void EngineCore::updateEssentials(const EditorMode& mode)
    {
        Scene* activeScene = sceneManager.GetActiveScene(mode);
        Scene* editorScene = sceneManager.GetEditorScene();


        inputManager.Update();
        inputRouter.update(deltaTime);

        assetManager.Update(*editorScene);
        UpdateTimer();


    }

    void EngineCore::updateRuntime(const EditorMode& mode)
    {
        Scene* runtimeScene = sceneManager.GetRuntimeScene().get();

        // !!! THIS FLOW MATTERS : controller -> movement -> physics -> scripts -> animation -> transform

        controllerSystem.Update(deltaTime);
        movementSystem.Update(deltaTime);
        physicsSystem.update(deltaTime, runtimeScene->GetRegistry().transforms);
        scriptSystem.OnUpdate(deltaTime);
        animationSystem.Update(runtimeScene->GetRegistry().animations, runtimeScene->GetRegistry().skeletons, deltaTime);
    }


    void EngineCore::pollEvents()
    {
        SDL_Event event;

        while (SDL_PollEvent(&event))
        {

            inputManager.processEvent(event);
            inputRouter.routeEvent(event);

            if (event.type == SDL_QUIT)
                running = false;
        }
    }

    void EngineCore::enterPlayMode()
    {
        sceneManager.CreateRuntimeScene();


        // Lock cursor 
        SDL_SetRelativeMouseMode(SDL_TRUE);

        // Route all keyboard/mouse to the game handler
        inputRouter.setContext(InputContext::Game);

    }

    void EngineCore::exitPlayMode()

    {
        scriptSystem.OnDestroy();

        // Restore cursor
        SDL_SetRelativeMouseMode(SDL_FALSE);

        // Return input focus to the editor UI
        inputRouter.setContext(InputContext::UI);

    }


    void EngineCore::run(const EditorMode mode)
    {
        updateEssentials(mode);
        pollEvents();

        if (mode == EditorMode::PLAY)
        {
            updateRuntime(mode);

            // Use the RUNTIME scene for transform propagation during play
            Scene* runtimeScene = sceneManager.GetRuntimeScene().get();
            transformSystem.Update(
                runtimeScene->GetRegistry().transforms,
                runtimeScene->GetRegistry().hierarchies,
                runtimeScene->GetRootEntities()
            );
        }
        else
        {
            // Editor mode — use the editor scene as before
            Scene* editorScene = sceneManager.GetEditorScene();
            transformSystem.Update(
                editorScene->GetRegistry().transforms,
                editorScene->GetRegistry().hierarchies,
                editorScene->GetRootEntities()
            );
        }
    }

    void EngineCore::presentFrame()
    {
        window.swapBuffer();
    }

    void EngineCore::shutdown()
    {
        assetManager.saveAssetDatabase();
        physicsSystem.shutdown();

        window.quitWindow();
    }

    void EngineCore::UpdateTimer()
    {
        runtimeStats.frameStats = LimitFPS(runtimeStats.targetFPS, runtimeStats.limitFPS);
        deltaTime = runtimeStats.frameStats.deltaTime;
    }

    bool& EngineCore::isRunning()
    {
        return running;
    }

    Window& EngineCore::getWindow()
    {
        return window;
    }

    InputManager& EngineCore::getInputManager()
    {
        return inputManager;
    }


    AssetManager& EngineCore::getAssetManager()
    {
        return assetManager;
    }

    SceneManager& EngineCore::getSceneManager()
    {
        return sceneManager;
    }

    RenderPipeline& EngineCore::getRenderPipeline()
    {
        return renderPipeline;
    }

    RenderSettings& EngineCore::getRenderSettings()
    {
        return renderSettings;
    }

    RuntimeStats& EngineCore::getRuntimeStats()
    {
        return runtimeStats;
    }

    PhysicsSystem& EngineCore::getPhysicsSystem()
    {
        return physicsSystem;
    }

    ScriptSystem& EngineCore::getScriptSystem()
    {
        return scriptSystem;
    }

}
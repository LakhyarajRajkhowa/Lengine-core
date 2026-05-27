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
        inputRouter(inputManager, gameEventSystem)

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

    }

    void EngineCore::updateEssentials(const EditorMode& mode)
    {
        Scene* activeScene = sceneManager.GetActiveScene(mode);
        Scene* editorScene = sceneManager.GetEditorScene();


        inputManager.Update();
        inputRouter.update(deltaTime);

        assetManager.Update(*editorScene);
        UpdateTimer();

        transformSystem.Update(activeScene->GetRegistry().transforms, activeScene->GetRegistry().hierarchies, activeScene->GetRootEntities());

    }

    void EngineCore::updateRuntime(const EditorMode& mode)
    {
        Scene* runtimeScene = sceneManager.GetActiveScene(mode);

        physicsSystem.update(deltaTime, runtimeScene->GetRegistry().transforms);
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

        // Notify game event subscribers
        gameEventSystem.dispatch(GameEventType::PlayModeEntered);
    }

    void EngineCore::exitPlayMode()
    {
        // Restore cursor
        SDL_SetRelativeMouseMode(SDL_FALSE);

        // Return input focus to the editor UI
        inputRouter.setContext(InputContext::UI);

        // Notify game event subscribers (allows cleanup of game-side state)
        gameEventSystem.dispatch(GameEventType::PlayModeExited);

        // Optionally clear game-only subscriptions so stale lambdas don't fire
        // Leave PlayModeEntered/Exited listeners alive for editor's use
        gameEventSystem.clearType(GameEventType::MoveAxis);
        gameEventSystem.clearType(GameEventType::LookAxis);
        gameEventSystem.clearType(GameEventType::Jump);
        gameEventSystem.clearType(GameEventType::Sprint);
        gameEventSystem.clearType(GameEventType::Interact);
        gameEventSystem.clearType(GameEventType::Attack);
        gameEventSystem.clearType(GameEventType::Pause);
    }


    void EngineCore::run(const EditorMode mode)
    {
        updateEssentials(mode);

        if(mode == EditorMode::PLAY)
            updateRuntime(mode);
        


        pollEvents();

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

}
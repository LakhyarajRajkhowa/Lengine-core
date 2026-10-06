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
        renderPipeline(assetManager, particleSystem),
        animationSystem(assetManager),
        boneSystem(assetManager),
        inputRouter(inputManager),
        controllerSystem(sceneManager, inputManager),
        movementSystem(sceneManager),
        scriptSystem(sceneManager, inputManager, physicsSystem, particleSystem, assetManager),
        particleSystem(assetManager)

    {
    }

    void EngineCore::initSystems()
    {
        InitTimer();

        renderSettings.resolution_X = settings.resolution_X;
        renderSettings.resolution_Y = settings.resolution_Y;
     
        assetManager.Init();

        std::vector<std::string> scenesTobeLoaded;
        scenesTobeLoaded.push_back("newScene");

        sceneManager.loadScenes(scenesTobeLoaded);

        assetManager.LoadSceneAssetRegistry(Paths::GameAssetRegistryFolder +
            sceneManager.GetEditorScene()->getName() +
            "_assets.json");

        renderPipeline.Init();

        physicsSystem.Init(*sceneManager.GetEditorScene());

        scriptSystem.Init(Paths::GameExecutableFolder +  "/GameScripts.dll");

        boneSystem.Init(*sceneManager.GetEditorScene());

        particleSystem.Init();

    }

    void EngineCore::updateEssentials(const EditorMode& mode)
    {
        Scene* activeScene = sceneManager.GetActiveScene(mode);
        Scene* editorScene = sceneManager.GetEditorScene();

        activeScene->Update();

        inputManager.Update();
        inputRouter.update(deltaTime);

        assetManager.Update(*editorScene);
        UpdateTimer();


    }

    void EngineCore::updateRuntime(const EditorMode& mode)
    {
        Scene* runtimeScene = sceneManager.GetRuntimeScene().get();
        auto& registry = runtimeScene->GetRegistry();

        scriptSystem.Update(deltaTime);

        controllerSystem.Update(deltaTime);
        movementSystem.Update(deltaTime);
        animationSystem.Update(registry.animations, registry.skeletons, deltaTime);
        physicsSystem.UpdateRuntime(deltaTime, registry.transforms);

        for (auto& e : physicsSystem.ConsumeCollisionEnterEvents())
            scriptSystem.OnCollisionEnter(e.a, e.b);
        for (auto& e : physicsSystem.ConsumeCollisionExitEvents())
            scriptSystem.OnCollisionExit(e.a, e.b);
        for (auto& e : physicsSystem.ConsumeTriggerEnterEvents())
            scriptSystem.OnTriggerEnter(e.a, e.b);
        for (auto& e : physicsSystem.ConsumeTriggerExitEvents())
            scriptSystem.OnTriggerExit(e.a, e.b);

        particleSystem.Update(deltaTime, registry.particleEmitters);

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
            Scene* runtimeScene = sceneManager.GetRuntimeScene().get();

            updateRuntime(mode);

            transformSystem.Update(
                runtimeScene->GetRegistry().transforms,
                runtimeScene->GetRegistry().hierarchies,
                runtimeScene->GetRootEntities()
            );

            boneSystem.Update(
                runtimeScene->GetRegistry().boneAttachments,
                runtimeScene->GetRegistry().animations,
                runtimeScene->GetRegistry().skeletons,
                runtimeScene->GetRegistry().transforms);
        }
        else
        {
            Scene* editorScene = sceneManager.GetEditorScene();
   
            transformSystem.Update(
                editorScene->GetRegistry().transforms,
                editorScene->GetRegistry().hierarchies,
                editorScene->GetRootEntities()
            );

            boneSystem.Update(
                editorScene->GetRegistry().boneAttachments,
                editorScene->GetRegistry().animations,
                editorScene->GetRegistry().skeletons,
                editorScene->GetRegistry().transforms);




        }
    }

    void EngineCore::presentFrame()
    {
        window.swapBuffer();
    }

    void EngineCore::shutdown()
    {
        assetManager.SaveAssetDatabase();
        physicsSystem.Shutdown();

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

    BoneAttachmentSystem& EngineCore::getBoneAttachmentSystem()
    {
        return boneSystem;
    }


    ScriptSystem& EngineCore::getScriptSystem()
    {
        return scriptSystem;
    }

    ParticleSystem& EngineCore::getParticleSystem()
    {
        return particleSystem;
    }

}
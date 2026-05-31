#pragma once

#include "ScriptLibrary.h"
#include "scene/SceneManager.h"
#include "input/InputManager.h"




namespace Lengine {

    class ScriptSystem
    {
    public:
        ScriptSystem(SceneManager& scn, InputManager& input) : sceneManager(scn), input(input) {}

        bool LoadLibrary(const std::string& dllPath);

        void OnCreate();
        void OnUpdate(float dt);
        void OnDestroy();

        void OnCollisionEnter(Entity a, Entity b);
        void OnCollisionExit(Entity a, Entity b);

        bool IsLibraryLoaded() const { return library.IsLoaded(); }

    private:

        SceneManager& sceneManager;
        InputManager& input;

        void InjectContext(ScriptableEntity& script, Entity entity,
            Registry& registry, InputManager& input);

        ScriptLibrary library;
        std::unordered_map<Entity, std::vector<ScriptableEntity*>> ownedScripts;


    };

} // namespace Lengine
#include "ScriptSystem.h"
#include <iostream>

namespace Lengine {

    bool ScriptSystem::LoadLibrary(const std::string& dllPath)
    {
        return library.Load(dllPath);
    }

    void ScriptSystem::InjectContext(ScriptableEntity& script,
        Entity entity,
        Registry& registry,
        InputManager& input)
    {
        script.entity = entity;
        script.registry = &registry;
        script.input = &input;
    }

    void ScriptSystem::OnCreate()
    {
        auto& scene = sceneManager.GetRuntimeScene();

        Registry& registry = scene->GetRegistry();

        const auto& entities = registry.scripts.GetEntities();

        for (Entity e : entities)
        {
            ScriptComponent& sc = registry.scripts.Get(e);

            for (const std::string& name : sc.scriptNames)
            {
                ScriptableEntity* s = library.Instantiate(name);
                if (!s) continue;

                ownedScripts[e].push_back(s);       // ScriptSystem owns it
                sc.scripts.push_back(s);          // component just points to it

                s->entity = e;
                s->registry = &registry;
                s->input = &input;
                s->OnCreate();
            }
        }
    }

    void ScriptSystem::OnUpdate(float dt)
    {
        auto& scene = sceneManager.GetRuntimeScene();

        Registry& registry = scene->GetRegistry();

        const auto& entities = registry.scripts.GetEntities();

       
        for (Entity e : entities)
        {
            ScriptComponent& sc = registry.scripts.Get(e);
            for (ScriptableEntity* s : sc.scripts)
                s->OnUpdate(dt);


        }

    }

    void ScriptSystem::OnDestroy()
    {
        auto& scene = sceneManager.GetRuntimeScene();

        Registry& registry = scene->GetRegistry();

        for (auto& [e, ptrs] : ownedScripts)
        {
            for (ScriptableEntity* s : ptrs)
            {
                s->OnDestroy();
                delete s;
            }

            if (registry.scripts.Has(e))
                registry.scripts.Get(e).scripts.clear();
        }

        ownedScripts.clear();
    }

    void ScriptSystem::OnCollisionEnter( Entity a, Entity b)
    {
        auto& scene = sceneManager.GetRuntimeScene();

        Registry& registry = scene->GetRegistry();

        auto notify = [&](Entity self, Entity other)
            {
                if (!registry.scripts.Has(self)) return;

                ScriptComponent& sc = registry.scripts.Get(self);
                for (auto& script : sc.scripts)
                {
                    if (script) script->OnCollisionEnter(other);
                }
            };

        notify(a, b);
        notify(b, a);
    }

    void ScriptSystem::OnCollisionExit(Entity a, Entity b)
    {
        auto& scene = sceneManager.GetRuntimeScene();

        Registry& registry = scene->GetRegistry();

        auto notify = [&](Entity self, Entity other)
            {
                if (!registry.scripts.Has(self)) return;

                ScriptComponent& sc = registry.scripts.Get(self);
                for (auto& script : sc.scripts)
                {
                    if (script) script->OnCollisionExit(other);
                }
            };

        notify(a, b);
        notify(b, a);
    }

} // namespace Lengine
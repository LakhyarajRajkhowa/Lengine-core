#include "ScriptSystem.h"
#include <iostream>

namespace Lengine {

    void ScriptSystem::Init(const std::string& dllPath) {
        library.Load(dllPath);
    }


    void ScriptSystem::Update(float dt) {
        CheckHotReload();

        OnUpdate(dt);
    }


    void ScriptSystem::CheckHotReload()
    {
        if (!library.IsLoaded()) return;

        if (!library.NeedsReload()) return;

        std::cout << "[ScriptSystem] Hot reload — restarting scripts\n";

        OnDestroy();          
        library.Load(library.GetPath());  
        OnCreate();           
    }


    void ScriptSystem::OnCreate()
    {
        auto& scene = sceneManager.GetRuntimeScene();
        Registry& registry = scene->GetRegistry();

        for (Entity e : registry.scripts.GetEntities())
        {
            ScriptComponent& sc = registry.scripts.Get(e);

            for (const std::string& name : sc.scriptNames)
            {
                ScriptableEntity* s = library.Instantiate(name);
                if (!s) continue;

                ownedScripts[e].push_back(s);
                sc.scripts.push_back(s);

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

        for (Entity e : registry.scripts.GetEntities())
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

    void ScriptSystem::OnCollisionEnter(Entity a, Entity b)
    {
        auto& scene = sceneManager.GetRuntimeScene();
        Registry& registry = scene->GetRegistry();

        auto notify = [&](Entity self, Entity other) {
            if (!registry.scripts.Has(self)) return;
            for (auto& s : registry.scripts.Get(self).scripts)
                if (s) s->OnCollisionEnter(other);
            };

        notify(a, b);
        notify(b, a);
    }

    void ScriptSystem::OnCollisionExit(Entity a, Entity b)
    {
        auto& scene = sceneManager.GetRuntimeScene();
        Registry& registry = scene->GetRegistry();

        auto notify = [&](Entity self, Entity other) {
            if (!registry.scripts.Has(self)) return;
            for (auto& s : registry.scripts.Get(self).scripts)
                if (s) s->OnCollisionExit(other);
            };

        notify(a, b);
        notify(b, a);
    }

} // namespace Lengine

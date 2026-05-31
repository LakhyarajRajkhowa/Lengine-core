#pragma once

#include "scene/Entity.h"   

namespace Lengine {

    class Registry;
    class InputManager;

    class ScriptableEntity
    {
    public:
        virtual ~ScriptableEntity() = default;

        virtual void OnCreate() {}
        virtual void OnUpdate(float dt) {}
        virtual void OnDestroy() {}
        virtual void OnCollisionEnter(Entity other) {}
        virtual void OnCollisionExit(Entity other) {}

        Entity          entity      = NullEntity;
        Registry*       registry    = nullptr;
        InputManager*   input       = nullptr;
    };

}

#ifdef _WIN32
    #define SCRIPT_API __declspec(dllexport)
#else
    #define SCRIPT_API
#endif

#define REGISTER_SCRIPT(ClassName)                                       \
    extern "C" SCRIPT_API Lengine::ScriptableEntity* Create_##ClassName() \
    {                                                                     \
        return new ClassName();                                           \
    }
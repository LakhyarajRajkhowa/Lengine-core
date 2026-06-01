#pragma once

#include "scene/Entity.h"

#include <string>
#include <vector>
#include <functional>

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

} // namespace Lengine


#ifdef _WIN32
    #define SCRIPT_API __declspec(dllexport)
#else
    #define SCRIPT_API __attribute__((visibility("default")))
#endif


namespace Lengine {
    namespace Internal {

        struct ScriptEntry {
            std::string name;      
            std::string sourceFile;
        };

        inline std::vector<ScriptEntry>& GetDLLRegistry()
        {
            static std::vector<ScriptEntry> s_registry;
            return s_registry;
        }

        struct ScriptRegistrar {
            ScriptRegistrar(const char* name, const char* file)
            {
                GetDLLRegistry().push_back({ name, file });
            }
        };

    }
} // namespace Lengine::Internal


#define REGISTER_SCRIPT(ClassName)                                                    \
    extern "C" SCRIPT_API Lengine::ScriptableEntity* Create_##ClassName()             \
    {                                                                                  \
        return new ClassName();                                                        \
    }                                                                                  \
    static Lengine::Internal::ScriptRegistrar                                         \
        s_registrar_##ClassName(#ClassName, __FILE__);


#define EXPORT_SCRIPT_REGISTRY()                                                      \
    extern "C" SCRIPT_API                                                             \
    void GetScriptRegistry(const char*** outNames,                                    \
                           const char*** outFiles,                                    \
                           int*          outCount)                                    \
    {                                                                                 \
        auto& reg = Lengine::Internal::GetDLLRegistry();                             \
        static std::vector<const char*> names, files;                                \
        names.clear(); files.clear();                                                \
        for (auto& e : reg) {                                                        \
            names.push_back(e.name.c_str());                                         \
            files.push_back(e.sourceFile.c_str());                                   \
        }                                                                             \
        *outNames = names.data();                                                     \
        *outFiles = files.data();                                                     \
        *outCount = static_cast<int>(reg.size());                                     \
    }
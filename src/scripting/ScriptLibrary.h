#pragma once

#include <string>
#include <functional>
#include <unordered_map>
#include <iostream>

#include "ScriptableEntity.h"


#ifdef _WIN32
#include <windows.h>
using LibHandle = HMODULE;
#else
#include <dlfcn.h>
using LibHandle = void*;
#endif

namespace Lengine {

    // Factory function signature 
    using ScriptFactory = ScriptableEntity * (*)();

    class ScriptLibrary
    {
    public:

        ScriptLibrary() = default;
        ~ScriptLibrary() { Unload(); }

        ScriptLibrary(const ScriptLibrary&) = delete;
        ScriptLibrary& operator=(const ScriptLibrary&) = delete;

        bool Load(const std::string& dllPath)
        {
            Unload();

#ifdef _WIN32
            handle = LoadLibraryA(dllPath.c_str());
            if (!handle)
            {
                std::cerr << "[ScriptLibrary] Failed to load: " << dllPath
                    << "  Error: " << GetLastError() << "\n";
                return false;
            }
#else
            m_Handle = dlopen(dllPath.c_str(), RTLD_NOW);
            if (!m_Handle)
            {
                std::cerr << "[ScriptLibrary] Failed to load: " << dllPath
                    << "\n  " << dlerror() << "\n";
                return false;
            }
#endif

            path = dllPath;
            std::cout << "[ScriptLibrary] Loaded: " << dllPath << "\n";
            return true;
        }

        void Unload()
        {
            if (!handle)
                return;

#ifdef _WIN32
            FreeLibrary(handle);
#else
            dlclose(m_Handle);
#endif
            handle = nullptr;
            path.clear();
            std::cout << "[ScriptLibrary] Unloaded dll\n";
        }

        bool IsLoaded() const { return handle != nullptr; }

        ScriptableEntity* Instantiate(const std::string& className) const
        {
            if (!handle)
            {
                std::cerr << "[ScriptLibrary] Instantiate called but no dll loaded\n";
                return nullptr;
            }

            std::string fnName = "Create_" + className;

#ifdef _WIN32
            auto fn = (ScriptFactory)GetProcAddress(handle, fnName.c_str());
#else
            auto fn = (ScriptFactory)dlsym(m_Handle, fnName.c_str());
#endif

            if (!fn)
            {
                std::cerr << "[ScriptLibrary] Factory not found: " << fnName
                    << "  (did you add REGISTER_SCRIPT(" << className << ") ?)\n";
                return nullptr;
            }

            return fn();
        }

        const std::string& GetPath() const { return path; }

    private:
        LibHandle   handle = nullptr;
        std::string path;
    };

} // namespace Lengine
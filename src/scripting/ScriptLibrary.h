#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <filesystem>

#include "ScriptableEntity.h"
#include "ScriptMetadata.h"

#ifdef _WIN32
    #include <windows.h>
    using LibHandle = HMODULE;
#else
    #include <dlfcn.h>
    using LibHandle = void*;
#endif

namespace Lengine {

    using ScriptFactory = ScriptableEntity * (*)();
    using FnGetScriptRegistry = void(*)(const char***, const char***, int*);

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

            m_SourcePath = dllPath;

            std::string loadPath = MakeTempCopy(dllPath);
            if (loadPath.empty()) return false;

            m_TempPath = loadPath;

#ifdef _WIN32
            m_Handle = LoadLibraryA(loadPath.c_str());
            if (!m_Handle) {
                std::cerr << "[ScriptLibrary] Failed to load: " << loadPath
                    << "  Error: " << GetLastError() << "\n";
                return false;
            }
#else
            m_Handle = dlopen(loadPath.c_str(), RTLD_NOW);
            if (!m_Handle) {
                std::cerr << "[ScriptLibrary] Failed to load: " << loadPath
                    << "\n  " << dlerror() << "\n";
                return false;
            }
#endif
            m_LastWriteTime = std::filesystem::last_write_time(dllPath);

            std::cout << "[ScriptLibrary] Loaded: " << dllPath
                << "  (copy: " << loadPath << ")\n";
            QueryRegistry();
            return true;
        }

        void Unload()
        {
            if (!m_Handle) return;

            m_Metadata.clear();

#ifdef _WIN32
            FreeLibrary(m_Handle);
#else
            dlclose(m_Handle);
#endif
            m_Handle = nullptr;

            // Delete the temp copy now that it's unloaded.
            if (!m_TempPath.empty()) {
                std::error_code ec;
                std::filesystem::remove(m_TempPath, ec);
                m_TempPath.clear();
            }
        }

        bool IsLoaded() const { return m_Handle != nullptr; }

        bool NeedsReload() const
        {
            if (m_SourcePath.empty()) return false;
            std::error_code ec;
            auto newTime = std::filesystem::last_write_time(m_SourcePath, ec);
            if (ec) return false;
            return newTime != m_LastWriteTime;
        }

        bool CheckAndReload()
        {
            if (!NeedsReload()) return false;
            return Load(m_SourcePath);
        }




        const std::vector<ScriptMetadata>& GetAllMetadata() const { return m_Metadata; }

        std::vector<std::string> GetScriptNames() const
        {
            std::vector<std::string> names;
            names.reserve(m_Metadata.size());
            for (auto& m : m_Metadata) names.push_back(m.name);
            return names;
        }

        const ScriptMetadata* FindMetadata(const std::string& className) const
        {
            for (auto& m : m_Metadata)
                if (m.name == className) return &m;
            return nullptr;
        }


        ScriptableEntity* Instantiate(const std::string& className) const
        {
            if (!m_Handle) {
                std::cerr << "[ScriptLibrary] Instantiate called but no dll loaded\n";
                return nullptr;
            }

            std::string fnName = "Create_" + className;
#ifdef _WIN32
            auto fn = (ScriptFactory)GetProcAddress(m_Handle, fnName.c_str());
#else
            auto fn = (ScriptFactory)dlsym(m_Handle, fnName.c_str());
#endif
            if (!fn) {
                std::cerr << "[ScriptLibrary] Factory not found: " << fnName
                    << "  (did you add REGISTER_SCRIPT(" << className << ") ?)\n";
                return nullptr;
            }
            return fn();
        }

        const std::string& GetPath() const { return m_SourcePath; }

    private:

        // Copies dllPath → dllPath.tmp_N.dll, returns the temp path.
        static std::string MakeTempCopy(const std::string& src)
        {
            // Build a unique name each reload so Windows doesn't complain about
            // overwriting a still-mapped file.
            static int s_counter = 0;
            std::filesystem::path p(src);
            std::string tempPath = p.parent_path().string() + "/" +
                p.stem().string() +
                "_tmp" + std::to_string(s_counter++) +
                p.extension().string();

            std::error_code ec;
            std::filesystem::copy_file(src,
                tempPath,
                std::filesystem::copy_options::overwrite_existing,
                ec);
            if (ec) {
                std::cerr << "[ScriptLibrary] Failed to copy DLL: "<< src << " ::" << ec.message() << "\n";
                return {};
            }
            return tempPath;
        }

        void QueryRegistry()
        {
            m_Metadata.clear();
#ifdef _WIN32
            auto fn = (FnGetScriptRegistry)GetProcAddress(m_Handle, "GetScriptRegistry");
#else
            auto fn = (FnGetScriptRegistry)dlsym(m_Handle, "GetScriptRegistry");
#endif
            if (!fn) {
                std::cerr << "[ScriptLibrary] GetScriptRegistry not found in DLL.\n"
                    << "  Did you add EXPORT_SCRIPT_REGISTRY() to one .cpp?\n";
                return;
            }

            const char** names = nullptr;
            const char** files = nullptr;
            int          count = 0;
            fn(&names, &files, &count);

            m_Metadata.reserve(count);
            for (int i = 0; i < count; ++i)
                m_Metadata.push_back({ names[i], files[i] });

            std::cout << "[ScriptLibrary] " << count << " script(s) registered:\n";
            for (auto& m : m_Metadata)
                std::cout << "  " << m.name << "  <-  " << m.sourceFile << "\n";
        }

        LibHandle                        m_Handle = nullptr;
        std::string                      m_SourcePath;   // original dll path
        std::string                      m_TempPath;     // loaded copy path
        std::filesystem::file_time_type  m_LastWriteTime;
        std::vector<ScriptMetadata>      m_Metadata;
    };

} // namespace Lengine

#pragma once

#include <vector>
#include <string>

namespace Lengine {

    class ScriptableEntity;

    struct ScriptComponent
    {
        std::vector<std::string>       scriptNames; // set in editor/scene setup
        std::vector<ScriptableEntity*> scripts;     // filled by ScriptSystem::OnCreate
    };

} // namespace Lengine
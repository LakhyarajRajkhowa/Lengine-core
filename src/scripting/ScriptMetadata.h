#pragma once

#include <string>

namespace Lengine {


    struct ScriptMetadata
    {
        std::string name;       
        std::string sourceFile; 

        bool IsValid() const { return !name.empty(); }
    };

} // namespace Lengine
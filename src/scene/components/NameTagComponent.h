#pragma once

#include <string>


namespace Lengine {
    class NameTagComponent
    {
    public:

        std::string name;

        NameTagComponent()
            : name("MyEntity")
        {
        }

        NameTagComponent(const std::string& name)
            : name(name)
        {
        }
    };
}

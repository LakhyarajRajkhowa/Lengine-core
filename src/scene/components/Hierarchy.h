#pragma once

#include "scene/Entity.h"

namespace Lengine {
    struct HierarchyComponent
    {
        Entity parent = NullEntity;
        std::vector<Entity> children;
    };

}

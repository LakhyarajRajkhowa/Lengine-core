#pragma once
#include <unordered_map>

#include "scene/Entity.h"

namespace Lengine
{

    struct MeshFilter
    {

        UUID meshID = UUID::Null;

        // async / deferred loading support
        bool pendingSubmesh = false;
        UUID pendingSubmeshID = UUID::Null;

        Entity rootParent = NullEntity;

        void RequestSubmesh(const UUID& id)
        {
            pendingSubmeshID = id;
            pendingSubmesh = true;
        }

        bool HasPendingSubmesh() const
        {
            return pendingSubmesh;
        }

        UUID GetRequestedSubmeshID() const
        {
            return pendingSubmeshID;
        }

        void ClearPendingSubmesh()
        {
            pendingSubmesh = false;
        }

        MeshFilter() = default;

        MeshFilter(UUID meshID, Entity rootParent):
            meshID(meshID),
            rootParent(rootParent)
        {}

    };

}

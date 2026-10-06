#pragma once
#include "BTTypes.h"
#include <memory>
#include <vector>

#include "scene/Entity.h"

namespace Lengine {

    class Registry;
    class BehaviorTreeRuntime;


    struct BTContext {
        Entity entity;
        Registry* registry;
        BehaviorTreeRuntime* runtime;  // gives leaf/composite access to per-entity state
        float dt;
    };

    // Base class for every node type. Nodes are BUILT ONCE as part of a
    // BehaviorTreeAsset and shared across every entity using that tree.
    // They must not contain per-entity mutable fields (e.g. "current child index") —
    // that lives in BehaviorTreeRuntime, looked up via m_ID.
    class BTNode {
    public:
        virtual ~BTNode() = default;

        virtual BTStatus Tick(BTContext& ctx) = 0;

        // Called when this node was Running last frame but a higher-priority
        // sibling took over this frame. Lets an action cancel in-flight work
        // (e.g. stop a PhysX sweep, cancel a path).
        virtual void OnAbort(BTContext& ctx) {}

        NodeID GetID() const { return m_ID; }
        void SetID(NodeID id) { m_ID = id; }

        virtual std::vector<BTNode*> GetChildrenForIDAssignment() { return {}; }

    protected:
        NodeID m_ID = kInvalidNodeID;
    };
}

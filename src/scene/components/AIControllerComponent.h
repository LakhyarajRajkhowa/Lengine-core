#pragma once
#include "ai/behaviorTree/BTAsset.h"
#include "ai/behaviorTree/BTRuntime.h"
#include <memory>

namespace Lengine {
    // The ECS component itself — attach this to any entity that should have
    // behavior-tree-driven AI (enemies, NPCs, etc.)
    struct AIControllerComponent {
        // Shared, immutable tree definition. Multiple entities of the same
        // enemy type point at the SAME BehaviorTreeAsset instance — this is
        // just a reference, not a copy.
        std::shared_ptr<BehaviorTreeAsset> asset;

        // Per-entity mutable state: blackboard data (current target, timers),
        // which composite child was Running, which leaf was Running last frame.
        // This is what's actually unique per entity.
        BehaviorTreeRuntime runtime;

        // Optional: pause this entity's AI without removing the component
        // (useful for cutscenes, scripted sequences, death state, etc.)
        bool enabled = true;
    };
}

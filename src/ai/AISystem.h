#pragma once
#include "scene/components/AIControllerComponent.h"
#include "scene/ECSRegistry.h"

namespace Lengine {

    class AISystem {
    public:
        void Update(Registry& registry, float dt) {

            auto& aiComponents = registry.aiControllers;
            auto& dense = aiComponents.GetDense();
            auto& entities = aiComponents.GetEntities();

            for (size_t i = 0; i < dense.size(); ++i)
            {
                AIControllerComponent& ai = dense[i];
                const Entity entity = entities[i];

                if (!ai.enabled) continue;

                BehaviorTreeRuntime& runtime = ai.runtime;

                // Shift last frame's "currently running leaf" into "last" slot,
                // then clear the current slot so this tick can set it fresh.
                runtime.lastRunningLeaf = runtime.currentRunningLeaf;
                runtime.currentRunningLeaf = kInvalidNodeID;

                BTContext ctx{ entity, &registry, &runtime, dt };
                ai.asset->GetRoot()->Tick(ctx);

                // If a different leaf (or none) ended up Running this frame,
                // the previous one was implicitly abandoned by a higher-priority
                // branch winning out -> notify it so it can cancel in-flight work.
                if (runtime.lastRunningLeaf != kInvalidNodeID &&
                    runtime.lastRunningLeaf != runtime.currentRunningLeaf) {

                    BTNode* abandoned = ai.asset->FindNodeByID(runtime.lastRunningLeaf);
                    if (abandoned) {
                        abandoned->OnAbort(ctx);
                    }
                }
            }
        }
    };
}

#pragma once
#include "ai/behaviorTree/BTNode.h"
#include "ai/behaviorTree/BTRuntime.h"

namespace Lengine {

    // Base class for every LEAF node — the actual "do something" nodes
    // (MoveTo, Wait, Attack, PlayAnimation, etc).
    //
    // This is what Sequence/Selector eventually bottom out into. It exists
    // as a separate class from BTNode because leaves have one extra
    // responsibility that composites don't: they are the ONLY nodes that
    // can produce a "currently running leaf" — composites just relay
    // whatever status their children gave them. That's why AISystem's
    // abort-notification logic keys off currentRunningLeaf, not off
    // "any node that returned Running".
    class BTAction : public BTNode {
    public:
        // Leaves override THIS instead of Tick() directly.
        virtual BTStatus OnTick(BTContext& ctx) = 0;

        // Leaves have no children by definition, so ID assignment naturally
        // stops here — no need to override GetChildrenForIDAssignment().

        BTStatus Tick(BTContext& ctx) final {
            BTStatus status = OnTick(ctx);

            if (status == BTStatus::Running) {
                // Record ourselves as "the leaf currently running" so
                // AISystem can detect next frame whether a higher-priority
                // branch preempted us, and so it knows WHICH node to call
                // OnAbort() on.
                ctx.runtime->currentRunningLeaf = m_ID;
            }

            return status;
        }
    };
}
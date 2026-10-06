#pragma once
#include "BTNode.h"
#include "BTRuntime.h"

namespace Lengine {

    class Sequence : public BTNode {
    public:
        std::vector<std::unique_ptr<BTNode>> children;

        std::vector<BTNode*> GetChildrenForIDAssignment() override {
            std::vector<BTNode*> out;
            out.reserve(children.size());
            for (auto& c : children) out.push_back(c.get());
            return out;
        }

        BTStatus Tick(BTContext& ctx) override {
            int startIndex = ctx.runtime->GetRunningChildIndex(m_ID);

            for (int i = startIndex; i < (int)children.size(); i++) {
                BTStatus status = children[i]->Tick(ctx);

                if (status == BTStatus::Running) {
                    ctx.runtime->SetRunningChildIndex(m_ID, i);
                    return BTStatus::Running;
                }
                if (status == BTStatus::Failure) {
                    ctx.runtime->ResetRunningChildIndex(m_ID);
                    return BTStatus::Failure;
                }
                // Success -> fall through to next child
            }

            ctx.runtime->ResetRunningChildIndex(m_ID);
            return BTStatus::Success;
        }
    };

    class Selector : public BTNode {
    public:
        std::vector<std::unique_ptr<BTNode>> children;

        std::vector<BTNode*> GetChildrenForIDAssignment() override {
            std::vector<BTNode*> out;
            out.reserve(children.size());
            for (auto& c : children) out.push_back(c.get());
            return out;
        }

        BTStatus Tick(BTContext& ctx) override {
            int startIndex = ctx.runtime->GetRunningChildIndex(m_ID);

            for (int i = startIndex; i < (int)children.size(); i++) {
                BTStatus status = children[i]->Tick(ctx);

                if (status == BTStatus::Running) {
                    ctx.runtime->SetRunningChildIndex(m_ID, i);
                    return BTStatus::Running;
                }
                if (status == BTStatus::Success) {
                    ctx.runtime->ResetRunningChildIndex(m_ID);
                    return BTStatus::Success;
                }
                // Failure -> fall through, try next child
            }

            ctx.runtime->ResetRunningChildIndex(m_ID);
            return BTStatus::Failure;
        }
    };

  
}

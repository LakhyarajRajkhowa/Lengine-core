#pragma once

#include "BTAction.h"

namespace Lengine {
    class Wait : public BTAction {
    public:
        explicit Wait(float duration) : m_Duration(duration) {}

        BTStatus OnTick(BTContext& ctx) override {
            std::string key = "wait_" + std::to_string(m_ID); // per-node key
            float* elapsed = ctx.runtime->GetValue<float>(key);

            if (!elapsed) {
                ctx.runtime->SetValue<float>(key, 0.0f);
                elapsed = ctx.runtime->GetValue<float>(key);
            }

            *elapsed += ctx.dt;

            if (*elapsed >= m_Duration) {
                ctx.runtime->ClearValue(key);
                return BTStatus::Success;
            }
            return BTStatus::Running;
        }

    private:
        float m_Duration;
    };
}
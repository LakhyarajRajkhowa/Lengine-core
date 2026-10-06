#pragma once
#include <cstdint>

namespace Lengine {
    enum class BTStatus : uint8_t {
        Success,
        Failure,
        Running
    };

    // Assigned once when the tree asset is built/loaded, never changes at runtime.
    // This is what lets BehaviorTreeRuntime track "where was I" per-entity
    // without the shared node itself holding any state.
    using NodeID = uint32_t;
    static constexpr NodeID kInvalidNodeID = 0;
}


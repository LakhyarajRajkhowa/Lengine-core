#pragma once
#include "BTTypes.h"
#include <unordered_map>
#include <unordered_map>
#include <any>
#include <string>


namespace Lengine {

    // Per-entity mutable state for a shared BehaviorTreeAsset.
    // Two responsibilities:
    //   1. Blackboard - arbitrary key/value scratch data (targets, timers, etc.)
    //   2. Composite progress - which child a Sequence/Selector was on
    //      when it last returned Running, keyed by that composite's NodeID.
    class BehaviorTreeRuntime {
    public:
        // ---- Blackboard ----
        template<typename T>
        void SetValue(const std::string& key, T value) {
            m_Blackboard[key] = std::move(value);
        }

        template<typename T>
        T* GetValue(const std::string& key) {
            auto it = m_Blackboard.find(key);
            if (it == m_Blackboard.end()) return nullptr;
            return std::any_cast<T>(&it->second);
        }

        void ClearValue(const std::string& key) {
            m_Blackboard.erase(key);
        }

        // ---- Composite child-progress tracking ----
        // A Sequence/Selector calls this instead of storing an index on itself.
        int GetRunningChildIndex(NodeID compositeID) const {
            auto it = m_RunningChild.find(compositeID);
            return it != m_RunningChild.end() ? it->second : 0;
        }

        void SetRunningChildIndex(NodeID compositeID, int index) {
            m_RunningChild[compositeID] = index;
        }

        void ResetRunningChildIndex(NodeID compositeID) {
            m_RunningChild.erase(compositeID);
        }

        // ---- Frame bookkeeping (for OnAbort) ----
        // Tracks which node was Running last frame vs. this frame so AISystem
        // can call OnAbort() on anything that dropped out.
        NodeID lastRunningLeaf = kInvalidNodeID;
        NodeID currentRunningLeaf = kInvalidNodeID;

    private:
        std::unordered_map<std::string, std::any> m_Blackboard;
        std::unordered_map<NodeID, int> m_RunningChild;
    };
}

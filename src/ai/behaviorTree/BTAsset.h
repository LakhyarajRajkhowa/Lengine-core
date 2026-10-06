#pragma once
#include "BTNode.h"
#include <memory>
#include <vector>
#include <unordered_map>

namespace Lengine {

    // Shared, immutable tree definition. Built once (from editor authoring or
    // a data file), then referenced by many AIControllerComponents.
    class BehaviorTreeAsset {
    public:
        explicit BehaviorTreeAsset(std::unique_ptr<BTNode> root)
            : m_Root(std::move(root))
        {
            NodeID nextID = 1; // 0 is reserved as kInvalidNodeID
            AssignIDs(m_Root.get(), nextID);
        }

        BTNode* GetRoot() const { return m_Root.get(); }

        // Used by AISystem to look up an abandoned Running leaf so it can
        // call OnAbort() on it. Populated during AssignIDs at construction.
        BTNode* FindNodeByID(NodeID id) const {
            auto it = m_NodeLookup.find(id);
            return it != m_NodeLookup.end() ? it->second : nullptr;
        }

    private:
        // Walks the tree once at construction time, giving every node a stable,
        // unique ID and recording it in m_NodeLookup. This is the ONLY place
        // NodeIDs get assigned — after this, they never change, which is what
        // makes them safe to use as lookup keys in BehaviorTreeRuntime across
        // frames, and safe for FindNodeByID to resolve later.
        void AssignIDs(BTNode* node, NodeID& nextID) {
            if (!node) return;
            node->SetID(nextID++);
            m_NodeLookup[node->GetID()] = node;

            for (BTNode* child : node->GetChildrenForIDAssignment()) {
                AssignIDs(child, nextID);
            }
        }

        std::unique_ptr<BTNode> m_Root;
        std::unordered_map<NodeID, BTNode*> m_NodeLookup;
    };
}
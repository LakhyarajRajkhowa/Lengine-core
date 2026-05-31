
#pragma once

#include <unordered_map>
#include <vector>
#include <cassert>
#include <algorithm>

#include "scene/Entity.h"

#include "MeshRenderer.h"
#include "MeshFilter.h"
#include "Transform.h"
#include "SkeletonComponent.h"
#include "AnimatonComponent.h"
#include "ColliderComponent.h"
#include "ControllerComponent.h"
#include "MovementComponent.h"
#include "RigidBodyComponent.h"
#include "NameTagComponent.h"
#include "CameraComponent.h"
#include "Hierarchy.h"
#include "Light.h"
#include "ScriptComponent.h"

namespace Lengine {

    template<typename T>
    class ComponentStorage
    {
    public:

        T& Add(const Entity& entityID, const T& component = T())
        {
            // If already has it, just update in-place
            if (Has(entityID))
            {
                uint32_t idx = m_Sparse[entityID];
                m_Dense[idx] = component;
                return m_Dense[idx];
            }

            if (entityID >= m_Sparse.size())
                m_Sparse.resize(entityID + 1, INVALID);

            m_Sparse[entityID] = static_cast<uint32_t>(m_Dense.size());

            m_Dense.push_back(component);
            m_DenseEntities.push_back(entityID);


            return m_Dense.back();
        }

        // Remove by swapping with the last element
        void Remove(const Entity& entityID)
        {
            if (!Has(entityID))
                return;

            uint32_t removedIdx = m_Sparse[entityID];
            uint32_t lastIdx = static_cast<uint32_t>(m_Dense.size()) - 1;

            if (removedIdx != lastIdx)
            {
                m_Dense[removedIdx] = std::move(m_Dense[lastIdx]);
                m_DenseEntities[removedIdx] = m_DenseEntities[lastIdx];

                Entity movedEntity = m_DenseEntities[removedIdx];
                m_Sparse[movedEntity] = removedIdx;
            }

            m_Dense.pop_back();
            m_DenseEntities.pop_back();

            m_Sparse[entityID] = INVALID;
        }


        bool Has(const Entity& entityID) const
        {
            return entityID < m_Sparse.size()
                && m_Sparse[entityID] != INVALID;
        }

        T& Get(const Entity& entityID)
        {
            assert(Has(entityID) && "Entity does not have this component");
            return m_Dense[m_Sparse[entityID]];
        }

        const T& Get(const Entity& entityID) const
        {
            assert(Has(entityID) && "Entity does not have this component");
            return m_Dense[m_Sparse[entityID]];
        }

        std::vector<T>& GetDense() { return m_Dense; }
        const std::vector<T>& GetDense() const { return m_Dense; }

        // Which entity owns dense[i]
        const std::vector<Entity>& GetEntities() const { return m_DenseEntities; }

        // Size = number of components currently alive
        size_t Size() const { return m_Dense.size(); }
        bool   Empty() const { return m_Dense.empty(); }

        void Clear()
        {
            m_Dense.clear();
            m_DenseEntities.clear();
            m_Sparse.clear();
        }



        void CloneFrom(
            const ComponentStorage<T>& src,
            const std::unordered_map<Entity, Entity>& entityMap)
        {
            
            Clear();
            for (size_t i = 0; i < src.m_Dense.size(); ++i)
            {
                Entity oldEntity = src.m_DenseEntities[i];
                auto   it = entityMap.find(oldEntity);
                if (it == entityMap.end())
                    continue;
                Add(it->second, src.m_Dense[i]);
            }
        }

    private:
        static constexpr uint32_t INVALID = std::numeric_limits<uint32_t>::max();

        std::vector<T>      m_Dense;          // component data, tightly packed
        std::vector<Entity> m_DenseEntities;  // entity[i] owns Dense[i]

        // m_Sparse[entityID] = index into m_Dense
        std::vector<uint32_t> m_Sparse;
    };

} // namespace Lengine
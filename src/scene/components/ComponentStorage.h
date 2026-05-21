#pragma once

#include <unordered_map>

#include "../utils/UUID.h"

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

namespace Lengine {

    template<typename T>
    class ComponentStorage
    {
    public:

        T& Add(
            const UUID& entityID,
            const T& component = T())
        {
            return m_Data[entityID] = component;
        }

        void Remove(const UUID& entityID)
        {
            m_Data.erase(entityID);
        }

        bool Has(const UUID& entityID) const
        {
            return m_Data.find(entityID) != m_Data.end();
        }

        T& Get(const UUID& entityID)
        {
            return m_Data.at(entityID);
        }

        const T& Get(const UUID& entityID) const
        {
            return m_Data.at(entityID);
        }

        std::unordered_map<UUID, T>& All()
        {
            return m_Data;
        }

        const std::unordered_map<UUID, T>& All() const
        {
            return m_Data;
        }

        void Clear()
        {
            m_Data.clear();
        }

        void CloneFrom(
            const ComponentStorage<T>& src,
            const std::unordered_map<UUID, UUID>& entityMap)
        {
            m_Data.clear();

            for (const auto& [oldEntity, component] : src.All())
            {
                auto it = entityMap.find(oldEntity);
                if (it == entityMap.end())
                    continue;

                UUID newEntity = it->second;
                m_Data[newEntity] = component;
            }
        }

    private:

        std::unordered_map<UUID, T> m_Data;
    };

}
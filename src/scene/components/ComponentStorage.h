#pragma once

#include <unordered_map>

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

namespace Lengine {

    template<typename T>
    class ComponentStorage
    {
    public:

        T& Add(
            const Entity& entityID,
            const T& component = T())
        {
            return m_Data[entityID] = component;
        }

        void Remove(const Entity& entityID)
        {
            m_Data.erase(entityID);
        }

        bool Has(const Entity& entityID) const
        {
            return m_Data.find(entityID) != m_Data.end();
        }

        T& Get(const Entity& entityID)
        {
            return m_Data.at(entityID);
        }

        const T& Get(const Entity& entityID) const
        {
            return m_Data.at(entityID);
        }

        std::unordered_map<Entity, T>& All()
        {
            return m_Data;
        }

        const std::unordered_map<Entity, T>& All() const
        {
            return m_Data;
        }

        void Clear()
        {
            m_Data.clear();
        }

        void CloneFrom(
            const ComponentStorage<T>& src,
            const std::unordered_map<Entity, Entity>& entityMap)
        {
            m_Data.clear();

            for (const auto& [oldEntity, component] : src.All())
            {
                auto it = entityMap.find(oldEntity);
                if (it == entityMap.end())
                    continue;

                Entity newEntity = it->second;
                m_Data[newEntity] = component;
            }
        }

    private:

        std::unordered_map<Entity, T> m_Data;
    };

}
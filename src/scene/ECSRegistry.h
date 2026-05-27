

#pragma once

#include <cstdint>
#include <vector>
#include <algorithm>

#include "scene/components/ComponentStorage.h"


namespace Lengine {

    class Registry
    {
    public:


        Entity CreateEntity()
        {
            Entity id = m_NextID++;
            m_Entities.push_back(id);
            return id;
        }

        void DestroyEntity(Entity id)
        {
            transforms.Remove(id);
            meshRenderers.Remove(id);
            meshFilters.Remove(id);
            skeletons.Remove(id);
            animations.Remove(id);
            colliders.Remove(id);
            controllers.Remove(id);
            movements.Remove(id);
            rigidBodies.Remove(id);
            nameTags.Remove(id);
            cameras.Remove(id);
            hierarchies.Remove(id);
            lights.Remove(id);

            auto it = std::find(m_Entities.begin(), m_Entities.end(), id);
            if (it != m_Entities.end())
                m_Entities.erase(it);
        }

        bool IsAlive(Entity id) const
        {
            return std::find(m_Entities.begin(), m_Entities.end(), id) != m_Entities.end();
        }

        const std::vector<Entity>& GetEntities() const { return m_Entities; }

        // Find the smallest storage size among all requested types
        template<typename... Ts>
        std::vector<Entity> View() const
        {
            std::vector<Entity> result;
            size_t minSize = std::min({ StorageSize<Ts>()... });

            ViewImpl<Ts...>(result, minSize);
            return result;
        }

        template<typename T> bool HasComponent(Entity e) const;
        template<typename T> T& GetComponent(Entity e);
        template<typename T> const T& GetComponent(Entity e) const;
        template<typename T> T& AddComponent(Entity e, const T& c = T());
        template<typename T> void RemoveComponent(Entity e);


        ComponentStorage<TransformComponent>  transforms;
        ComponentStorage<MeshRenderer>        meshRenderers;
        ComponentStorage<MeshFilter>          meshFilters;
        ComponentStorage<SkeletonComponent>   skeletons;
        ComponentStorage<AnimationComponent>  animations;
        ComponentStorage<ColliderComponent>   colliders;
        ComponentStorage<ControllerComponent> controllers;
        ComponentStorage<MovementComponent>   movements;
        ComponentStorage<RigidbodyComponent>  rigidBodies;
        ComponentStorage<NameTagComponent>    nameTags;
        ComponentStorage<CameraComponent>     cameras;
        ComponentStorage<HierarchyComponent>  hierarchies;
        ComponentStorage<Light>               lights;

        void Clear()
        {
            m_Entities.clear();
            m_NextID = 1;
            transforms.Clear();    meshRenderers.Clear();  meshFilters.Clear();
            skeletons.Clear();     animations.Clear();     colliders.Clear();
            controllers.Clear();   movements.Clear();      rigidBodies.Clear();
            nameTags.Clear();      cameras.Clear();        hierarchies.Clear();
            lights.Clear();
        }


        template<typename T>
        const std::vector<Entity>& GetStorageEntities() const;

    private:
        std::vector<Entity> m_Entities;
        Entity              m_NextID = 1;

        template<typename T> size_t StorageSize() const;

        // Walks the entities of the smallest storage, filters by the rest
        template<typename First, typename... Rest>
        void ViewImpl(std::vector<Entity>& out, size_t minSize) const
        {
            if (StorageSize<First>() == minSize)
            {
                const auto& entities = GetStorageEntities<First>();
                out.reserve(entities.size());
                for (Entity e : entities)
                {
                    if ((HasComponent<Rest>(e) && ...))
                        out.push_back(e);
                }
                return;
            }
            if constexpr (sizeof...(Rest) > 0)
                ViewImpl<Rest...>(out, minSize);
        }

    };


    template<> inline size_t Registry::StorageSize<TransformComponent>()  const { return transforms.Size(); }
    template<> inline size_t Registry::StorageSize<MeshRenderer>()        const { return meshRenderers.Size(); }
    template<> inline size_t Registry::StorageSize<MeshFilter>()          const { return meshFilters.Size(); }
    template<> inline size_t Registry::StorageSize<SkeletonComponent>()   const { return skeletons.Size(); }
    template<> inline size_t Registry::StorageSize<AnimationComponent>()  const { return animations.Size(); }
    template<> inline size_t Registry::StorageSize<ColliderComponent>()   const { return colliders.Size(); }
    template<> inline size_t Registry::StorageSize<ControllerComponent>() const { return controllers.Size(); }
    template<> inline size_t Registry::StorageSize<MovementComponent>()   const { return movements.Size(); }
    template<> inline size_t Registry::StorageSize<RigidbodyComponent>()  const { return rigidBodies.Size(); }
    template<> inline size_t Registry::StorageSize<NameTagComponent>()    const { return nameTags.Size(); }
    template<> inline size_t Registry::StorageSize<CameraComponent>()     const { return cameras.Size(); }
    template<> inline size_t Registry::StorageSize<HierarchyComponent>()  const { return hierarchies.Size(); }
    template<> inline size_t Registry::StorageSize<Light>()               const { return lights.Size(); }


    template<> inline const std::vector<Entity>& Registry::GetStorageEntities<TransformComponent>()  const { return transforms.GetEntities(); }
    template<> inline const std::vector<Entity>& Registry::GetStorageEntities<MeshRenderer>()        const { return meshRenderers.GetEntities(); }
    template<> inline const std::vector<Entity>& Registry::GetStorageEntities<MeshFilter>()          const { return meshFilters.GetEntities(); }
    template<> inline const std::vector<Entity>& Registry::GetStorageEntities<SkeletonComponent>()   const { return skeletons.GetEntities(); }
    template<> inline const std::vector<Entity>& Registry::GetStorageEntities<AnimationComponent>()  const { return animations.GetEntities(); }
    template<> inline const std::vector<Entity>& Registry::GetStorageEntities<ColliderComponent>()   const { return colliders.GetEntities(); }
    template<> inline const std::vector<Entity>& Registry::GetStorageEntities<ControllerComponent>() const { return controllers.GetEntities(); }
    template<> inline const std::vector<Entity>& Registry::GetStorageEntities<MovementComponent>()   const { return movements.GetEntities(); }
    template<> inline const std::vector<Entity>& Registry::GetStorageEntities<RigidbodyComponent>()  const { return rigidBodies.GetEntities(); }
    template<> inline const std::vector<Entity>& Registry::GetStorageEntities<NameTagComponent>()    const { return nameTags.GetEntities(); }
    template<> inline const std::vector<Entity>& Registry::GetStorageEntities<CameraComponent>()     const { return cameras.GetEntities(); }
    template<> inline const std::vector<Entity>& Registry::GetStorageEntities<HierarchyComponent>()  const { return hierarchies.GetEntities(); }
    template<> inline const std::vector<Entity>& Registry::GetStorageEntities<Light>()               const { return lights.GetEntities(); }


    template<> inline bool Registry::HasComponent<TransformComponent>(Entity e) const { return transforms.Has(e); }
    template<> inline TransformComponent& Registry::GetComponent<TransformComponent>(Entity e) { return transforms.Get(e); }
    template<> inline TransformComponent& Registry::AddComponent<TransformComponent>(Entity e, const TransformComponent& c) { return transforms.Add(e, c); }
    template<> inline void Registry::RemoveComponent<TransformComponent>(Entity e) { transforms.Remove(e); }

    template<> inline bool Registry::HasComponent<MeshRenderer>(Entity e) const { return meshRenderers.Has(e); }
    template<> inline MeshRenderer& Registry::GetComponent<MeshRenderer>(Entity e) { return meshRenderers.Get(e); }
    template<> inline MeshRenderer& Registry::AddComponent<MeshRenderer>(Entity e, const MeshRenderer& c) { return meshRenderers.Add(e, c); }
    template<> inline void Registry::RemoveComponent<MeshRenderer>(Entity e) { meshRenderers.Remove(e); }

    template<> inline bool Registry::HasComponent<MeshFilter>(Entity e) const { return meshFilters.Has(e); }
    template<> inline MeshFilter& Registry::GetComponent<MeshFilter>(Entity e) { return meshFilters.Get(e); }
    template<> inline MeshFilter& Registry::AddComponent<MeshFilter>(Entity e, const MeshFilter& c) { return meshFilters.Add(e, c); }
    template<> inline void Registry::RemoveComponent<MeshFilter>(Entity e) { meshFilters.Remove(e); }

    template<> inline bool Registry::HasComponent<SkeletonComponent>(Entity e) const { return skeletons.Has(e); }
    template<> inline SkeletonComponent& Registry::GetComponent<SkeletonComponent>(Entity e) { return skeletons.Get(e); }
    template<> inline SkeletonComponent& Registry::AddComponent<SkeletonComponent>(Entity e, const SkeletonComponent& c) { return skeletons.Add(e, c); }
    template<> inline void Registry::RemoveComponent<SkeletonComponent>(Entity e) { skeletons.Remove(e); }

    template<> inline bool Registry::HasComponent<AnimationComponent>(Entity e) const { return animations.Has(e); }
    template<> inline AnimationComponent& Registry::GetComponent<AnimationComponent>(Entity e) { return animations.Get(e); }
    template<> inline AnimationComponent& Registry::AddComponent<AnimationComponent>(Entity e, const AnimationComponent& c) { return animations.Add(e, c); }
    template<> inline void Registry::RemoveComponent<AnimationComponent>(Entity e) { animations.Remove(e); }

    template<> inline bool Registry::HasComponent<ColliderComponent>(Entity e) const { return colliders.Has(e); }
    template<> inline ColliderComponent& Registry::GetComponent<ColliderComponent>(Entity e) { return colliders.Get(e); }
    template<> inline ColliderComponent& Registry::AddComponent<ColliderComponent>(Entity e, const ColliderComponent& c) { return colliders.Add(e, c); }
    template<> inline void Registry::RemoveComponent<ColliderComponent>(Entity e) { colliders.Remove(e); }

    template<> inline bool Registry::HasComponent<ControllerComponent>(Entity e) const { return controllers.Has(e); }
    template<> inline ControllerComponent& Registry::GetComponent<ControllerComponent>(Entity e) { return controllers.Get(e); }
    template<> inline ControllerComponent& Registry::AddComponent<ControllerComponent>(Entity e, const ControllerComponent& c) { return controllers.Add(e, c); }
    template<> inline void Registry::RemoveComponent<ControllerComponent>(Entity e) { controllers.Remove(e); }

    template<> inline bool Registry::HasComponent<MovementComponent>(Entity e) const { return movements.Has(e); }
    template<> inline MovementComponent& Registry::GetComponent<MovementComponent>(Entity e) { return movements.Get(e); }
    template<> inline MovementComponent& Registry::AddComponent<MovementComponent>(Entity e, const MovementComponent& c) { return movements.Add(e, c); }
    template<> inline void Registry::RemoveComponent<MovementComponent>(Entity e) { movements.Remove(e); }

    template<> inline bool Registry::HasComponent<RigidbodyComponent>(Entity e) const { return rigidBodies.Has(e); }
    template<> inline RigidbodyComponent& Registry::GetComponent<RigidbodyComponent>(Entity e) { return rigidBodies.Get(e); }
    template<> inline RigidbodyComponent& Registry::AddComponent<RigidbodyComponent>(Entity e, const RigidbodyComponent& c) { return rigidBodies.Add(e, c); }
    template<> inline void Registry::RemoveComponent<RigidbodyComponent>(Entity e) { rigidBodies.Remove(e); }

    template<> inline bool Registry::HasComponent<NameTagComponent>(Entity e) const { return nameTags.Has(e); }
    template<> inline NameTagComponent& Registry::GetComponent<NameTagComponent>(Entity e) { return nameTags.Get(e); }
    template<> inline NameTagComponent& Registry::AddComponent<NameTagComponent>(Entity e, const NameTagComponent& c) { return nameTags.Add(e, c); }
    template<> inline void Registry::RemoveComponent<NameTagComponent>(Entity e) { nameTags.Remove(e); }

    template<> inline bool Registry::HasComponent<CameraComponent>(Entity e) const { return cameras.Has(e); }
    template<> inline CameraComponent& Registry::GetComponent<CameraComponent>(Entity e) { return cameras.Get(e); }
    template<> inline CameraComponent& Registry::AddComponent<CameraComponent>(Entity e, const CameraComponent& c) { return cameras.Add(e, c); }
    template<> inline void Registry::RemoveComponent<CameraComponent>(Entity e) { cameras.Remove(e); }

    template<> inline bool Registry::HasComponent<HierarchyComponent>(Entity e) const { return hierarchies.Has(e); }
    template<> inline HierarchyComponent& Registry::GetComponent<HierarchyComponent>(Entity e) { return hierarchies.Get(e); }
    template<> inline HierarchyComponent& Registry::AddComponent<HierarchyComponent>(Entity e, const HierarchyComponent& c) { return hierarchies.Add(e, c); }
    template<> inline void Registry::RemoveComponent<HierarchyComponent>(Entity e) { hierarchies.Remove(e); }

    template<> inline bool Registry::HasComponent<Light>(Entity e) const { return lights.Has(e); }
    template<> inline Light& Registry::GetComponent<Light>(Entity e) { return lights.Get(e); }
    template<> inline Light& Registry::AddComponent<Light>(Entity e, const Light& c) { return lights.Add(e, c); }
    template<> inline void Registry::RemoveComponent<Light>(Entity e) { lights.Remove(e); }

    // for the const version of Get()
    template<> inline const TransformComponent& Registry::GetComponent<TransformComponent>(Entity e) const { return transforms.Get(e); }
    template<> inline const MeshRenderer& Registry::GetComponent<MeshRenderer>(Entity e) const { return meshRenderers.Get(e); }
    template<> inline const MeshFilter& Registry::GetComponent<MeshFilter>(Entity e) const { return meshFilters.Get(e); }
    template<> inline const SkeletonComponent& Registry::GetComponent<SkeletonComponent>(Entity e) const { return skeletons.Get(e); }
    template<> inline const AnimationComponent& Registry::GetComponent<AnimationComponent>(Entity e) const { return animations.Get(e); }
    template<> inline const ColliderComponent& Registry::GetComponent<ColliderComponent>(Entity e) const { return colliders.Get(e); }
    template<> inline const ControllerComponent& Registry::GetComponent<ControllerComponent>(Entity e) const { return controllers.Get(e); }
    template<> inline const MovementComponent& Registry::GetComponent<MovementComponent>(Entity e) const { return movements.Get(e); }
    template<> inline const RigidbodyComponent& Registry::GetComponent<RigidbodyComponent>(Entity e) const { return rigidBodies.Get(e); }
    template<> inline const NameTagComponent& Registry::GetComponent<NameTagComponent>(Entity e) const { return nameTags.Get(e); }
    template<> inline const CameraComponent& Registry::GetComponent<CameraComponent>(Entity e) const { return cameras.Get(e); }
    template<> inline const HierarchyComponent& Registry::GetComponent<HierarchyComponent>(Entity e) const { return hierarchies.Get(e); }
    template<> inline const Light& Registry::GetComponent<Light>(Entity e) const { return lights.Get(e); }

} // namespace Lengine
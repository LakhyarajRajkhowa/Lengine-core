#pragma once

#include "resources/TextureCache.h"

#include "scene/Entity.h"
#include "scene/components/ComponentStorage.h"

#include "assets/MaterialRegistry.h"

#include "transform/TransformSystem.h"
#include "physics/PhysicsSystem.h"
namespace Lengine {
    class Scene {
    public:
        
        Scene(const std::string& n, UUID sID)
            : name(n), sceneID(sID) 
        {
        }
  
        Entity* createEntity_root(
            const std::string& name,
            UUID entityID = UUID()
        );

        Entity* createEntity(
            const std::string& name,
            UUID entityID = UUID()
        );

        UUID GetRootParent(const UUID& entityID);
        UUID DuplicateEntityRecursive(UUID originalID, UUID newParent, UUID newRoot);
        UUID DuplicateHierarchy(UUID rootID);

        Entity* getEntityByID(const UUID& UUID);
        Entity* addEntity(std::unique_ptr<Entity> entity, const UUID originalEntityId);
       
        const  Entity* getEntityByID(const UUID& id) const;


        const std::vector<std::unique_ptr<Entity>>& getEntities() const { return entities; }
        std::vector<std::unique_ptr<Entity>>& getEntities() {
            return entities;
        }

        const std::vector<UUID>& GetRootEntities() const { return rootEntities; }
        std::vector<UUID>& GetRootEntities() {
            return rootEntities;
        }

        bool IsRootEntity(const UUID& id)
        {
            const auto& roots = GetRootEntities();
            return std::find(roots.begin(), roots.end(), id) != roots.end();
        }


        std::string getName() { return name; }
        const std::string& getName() const { return name; }
        void rename(const std::string newName) { name = newName; }

        UUID getUUID() const { return sceneID; }

        bool HasChildren(UUID entityID) const;
        const std::vector<UUID>& GetChildren(UUID entityID) const;

        void SetParent(UUID child, UUID parent);
        void MakeOrphan(UUID child);
        void RemoveEntity(const UUID);
        void RemoveEntityRecursive(UUID id);

        std::unique_ptr<Scene> Clone();
       
 
        const ComponentStorage<MeshRenderer>& MeshRenderers() const
        {
            return meshRenderers;
        }

        ComponentStorage<MeshRenderer>& MeshRenderers()
        {
            return meshRenderers;
        }
        const  ComponentStorage<MeshFilter>& MeshFilters() const {
            return meshFilters;
        }

        ComponentStorage<MeshFilter>& MeshFilters() {
            return meshFilters;
        }

        const ComponentStorage<TransformComponent>& Transforms() const
        {
            return transforms;
        }

        ComponentStorage<TransformComponent>& Transforms()
        {
            return transforms;
        }


        const ComponentStorage<HierarchyComponent>& Hierarchys() const
        {
            return hierarchys;
        }

        ComponentStorage<HierarchyComponent>& Hierarchys()
        {
            return hierarchys;
        }


        const ComponentStorage<NameTagComponent>& NameTags() const
        {
            return nameTags;
        }

        ComponentStorage<NameTagComponent>& NameTags()
        {
            return nameTags;
        }


        const ComponentStorage<SkeletonComponent>& Skeletons() const
        {
            return skeletons;
        }

        ComponentStorage<SkeletonComponent>& Skeletons()
        {
            return skeletons;
        }


        const ComponentStorage<AnimationComponent>& Animations() const
        {
            return animations;
        }

        ComponentStorage<AnimationComponent>& Animations()
        {
            return animations;
        }


        const ComponentStorage<CameraComponent>& Cameras() const
        {
            return cameras;
        }

        ComponentStorage<CameraComponent>& Cameras()
        {
            return cameras;
        }


        const ComponentStorage<ControllerComponent>& Controllers() const
        {
            return controllers;
        }

        ComponentStorage<ControllerComponent>& Controllers()
        {
            return controllers;
        }


        const ComponentStorage<ColliderComponent>& Colliders() const
        {
            return colliders;
        }

        ComponentStorage<ColliderComponent>& Colliders()
        {
            return colliders;
        }


        const ComponentStorage<RigidbodyComponent>& Rigidbodies() const
        {
            return rigidbodies;
        }

        ComponentStorage<RigidbodyComponent>& Rigidbodies()
        {
            return rigidbodies;
        }


        const ComponentStorage<Light>& Lights() const
        {
            return lights;
        }

        ComponentStorage<Light>& Lights()
        {
            return lights;
        }


        const UUID& GetDirectionalShadowCaster() const {
            return directionalShadowCaster;
        }

        void Scene::SetDirectionalShadowCaster(UUID entity)
        {
            // Clear old one
            if (directionalShadowCaster != UUID::Null &&
                lights.Has(directionalShadowCaster))
            {
                lights.Get(directionalShadowCaster).castShadow = false;
            }

            directionalShadowCaster = entity;

            // Set new one
            if (directionalShadowCaster != UUID::Null &&
                lights.Has(directionalShadowCaster))
            {
                lights.Get(directionalShadowCaster).castShadow = true;
            }
        }

        const UUID& GetPointShadowCaster() const {
            return pointShadowCaster;
        }

        void Scene::SetPointShadowCaster(UUID entity)
        {
            if (pointShadowCaster != UUID::Null &&
                lights.Has(pointShadowCaster))
            {
                lights.Get(pointShadowCaster).castShadow = false;
            }

            pointShadowCaster = entity;

            if (pointShadowCaster != UUID::Null &&
                lights.Has(pointShadowCaster))
            {
                lights.Get(pointShadowCaster).castShadow = true;
            }
        }

        const UUID& GetPrimaryCamera() const {
            return primaryCamera;
        }

        void SetPrimaryCamera(const UUID& id) {
            primaryCamera = id;
        }
        
        std::string GenerateDuplicateName(Scene* scene, const std::string& baseName);

    private:
        std::string name;
        UUID sceneID;
        std::vector<std::unique_ptr<Entity>> entities;
        std::vector<UUID> rootEntities;

        UUID primaryCamera = UUID::Null;
        UUID directionalShadowCaster = UUID::Null;
        UUID pointShadowCaster = UUID::Null;

        ComponentStorage<Light> lights;
        ComponentStorage<HierarchyComponent> hierarchys;
        ComponentStorage<CameraComponent> cameras;
        ComponentStorage<MeshRenderer> meshRenderers;
        ComponentStorage<MeshFilter> meshFilters;
        ComponentStorage<TransformComponent> transforms;
        ComponentStorage<NameTagComponent> nameTags;
        ComponentStorage<SkeletonComponent> skeletons;
        ComponentStorage<AnimationComponent> animations;
        ComponentStorage<ControllerComponent> controllers;
        ComponentStorage<ColliderComponent> colliders;
        ComponentStorage<RigidbodyComponent> rigidbodies;

    };
}


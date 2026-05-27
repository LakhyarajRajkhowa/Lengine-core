#pragma once

#include "resources/TextureCache.h"

#include "scene/Entity.h"
#include "scene/ECSRegistry.h"

#include "assets/MaterialRegistry.h"

#include "transform/TransformSystem.h"
namespace Lengine {
    class Scene {
    public:
        
        Scene(const std::string& n, UUID sID)
            : name(n), sceneID(sID) 
        {
        }
  
        Entity* createEntity_root(
            const std::string& name
        );

        Entity* createEntity(
            const std::string& name
        );

        Entity GetRootParent(const Entity& entityID);
        Entity DuplicateEntityRecursive(Entity originalID, Entity newParent, Entity newRoot);
        Entity DuplicateHierarchy(Entity rootID);

        Entity* getEntityByID(const Entity& UUID);
        Entity* addEntity(std::unique_ptr<Entity> entity, const Entity originalEntityId);
       
        const  Entity* getEntityByID(const Entity& id) const;


        const std::vector<std::unique_ptr<Entity>>& getEntities() const { return entities; }
        std::vector<std::unique_ptr<Entity>>& getEntities() {
            return entities;
        }

        const std::vector<Entity>& GetRootEntities() const { return rootEntities; }
        std::vector<Entity>& GetRootEntities() {
            return rootEntities;
        }

        bool IsRootEntity(const Entity& id)
        {
            const auto& roots = GetRootEntities();
            return std::find(roots.begin(), roots.end(), id) != roots.end();
        }


        std::string getName() { return name; }
        const std::string& getName() const { return name; }
        void rename(const std::string newName) { name = newName; }

        UUID getUUID() const { return sceneID; }

        bool HasChildren(Entity entityID) const;
        const std::vector<Entity>& GetChildren(Entity entityID) const;

        void SetParent(Entity child, Entity parent);
        void MakeOrphan(Entity child);
        void RemoveEntity(const Entity);
        void RemoveEntityRecursive(Entity id);

        std::unique_ptr<Scene> Clone();
       
        // Scene.h — just this
        const Registry& GetRegistry() const { return registry; }
        Registry& GetRegistry() { return registry; }

        const Entity& GetDirectionalShadowCaster() const {
            return directionalShadowCaster;
        }

        void SetDirectionalShadowCaster(Entity entity)
        {
            // Clear old one
            if (directionalShadowCaster != UUID::Null &&
                registry.lights.Has(directionalShadowCaster))
            {
                registry.lights.Get(directionalShadowCaster).castShadow = false;
            }

            directionalShadowCaster = entity;

            // Set new one
            if (directionalShadowCaster != UUID::Null &&
                registry.lights.Has(directionalShadowCaster))
            {
                registry.lights.Get(directionalShadowCaster).castShadow = true;
            }
        }

        const Entity& GetPointShadowCaster() const {
            return pointShadowCaster;
        }

        void SetPointShadowCaster(Entity entity)
        {
            if (pointShadowCaster != NullEntity &&
                registry.lights.Has(pointShadowCaster))
            {
                registry.lights.Get(pointShadowCaster).castShadow = false;
            }

            pointShadowCaster = entity;

            if (pointShadowCaster != NullEntity &&
                registry.lights.Has(pointShadowCaster))
            {
                registry.lights.Get(pointShadowCaster).castShadow = true;
            }
        }

        const Entity& GetPrimaryCamera() const {
            return primaryCamera;
        }

        void SetPrimaryCamera(const Entity& id) {
            primaryCamera = id;
        }
        
        std::string GenerateDuplicateName(Scene* scene, const std::string& baseName);

    private:
        std::string name;
        UUID sceneID;
        std::vector<std::unique_ptr<Entity>> entities;
        std::vector<Entity> rootEntities;

        Entity nextEntityID = 1;

        Entity primaryCamera = NullEntity;
        Entity directionalShadowCaster = NullEntity;
        Entity pointShadowCaster = NullEntity;
        
        Registry registry;

    };
}


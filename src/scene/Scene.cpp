#include "Scene.h"
#include "utils/C++20.h"
#include <iostream>

using namespace Lengine;

Entity* Scene::createEntity_root(const std::string& name)
{
    Entity id = nextEntityID++;
    auto entity = std::make_unique<Entity>(id);
    registry.nameTags.Add(id, NameTagComponent(name));

    Entity* entityPtr = entity.get();
    entities.push_back(std::move(entity));
    rootEntities.push_back(id);

    return entityPtr;
}

Entity* Scene::createEntity(const std::string& name)
{
    Entity id = nextEntityID++;
    auto entity = std::make_unique<Entity>(id);
    registry.nameTags.Add(id, NameTagComponent(name));

    Entity* entityPtr = entity.get();
    entities.push_back(std::move(entity));

    return entityPtr;
}

Entity Scene::DuplicateEntityRecursive(Entity originalID, Entity newParent, Entity newRoot)
{
    auto entity = std::make_unique<Entity>(nextEntityID++);

    Entity* newEntity = addEntity(std::move(entity), originalID);
    Entity newID = *newEntity;

    if (newRoot == NullEntity)
        newRoot = newID;

    if (newParent != NullEntity)
    {
        auto& h = registry.hierarchies.Add(newID);
        h.parent = newParent;
        registry.hierarchies.Get(newParent).children.push_back(newID);

        rootEntities.erase(
            std::remove(rootEntities.begin(), rootEntities.end(), newID),
            rootEntities.end()
        );
    }
    else
    {
        registry.hierarchies.Add(newID);
    }

    if (registry.meshFilters.Has(newID))
    {
        registry.meshFilters.Get(newID).rootParent = newRoot;
    }

    if (registry.hierarchies.Has(originalID))
    {
        const auto& children = registry.hierarchies.Get(originalID).children;

        for (Entity child : children)
        {
            DuplicateEntityRecursive(child, newID, newRoot);
        }
    }

    return newID;
}

Entity Scene::DuplicateHierarchy(Entity rootID)
{
    return DuplicateEntityRecursive(rootID, NullEntity, NullEntity);
}

Entity* Scene::addEntity(std::unique_ptr<Entity> entity, const Entity originalEntityId)
{
    if (!entity)
        return nullptr;

    if (*entity == NullEntity)
    {
        *entity = nextEntityID++;
    }

    Entity entityId = *entity;

    if (registry.transforms.Has(originalEntityId)) {
        const TransformComponent& oldTrans = registry.transforms.Get(originalEntityId);
        TransformComponent newTrans = TransformComponent(oldTrans);
        registry.transforms.Add(entityId, newTrans);

        auto& t = registry.transforms.Get(entityId);
        glm::vec3 offset = glm::vec3{ (t.GetWorldScale().x * 1.0f), 0.0f, (t.GetWorldScale().z * 1.0f) };

        t.localPosition += offset;
        t.localDirty = true;

        TransformSystem::Dirty = true;
    }

    if (registry.meshFilters.Has(originalEntityId)) {
        const MeshFilter& oldMf = registry.meshFilters.Get(originalEntityId);
        registry.meshFilters.Add(entityId, MeshFilter(oldMf.meshID, entityId));

        std::cout << " Scene.cpp :: root: " << GetRootParent(entityId) << std::endl;
    }

    if (registry.meshRenderers.Has(originalEntityId)) {
        const MeshRenderer& oldMr = registry.meshRenderers.Get(originalEntityId);
        MeshRenderer newMr = MeshRenderer(oldMr);
        registry.meshRenderers.Add(entityId, newMr);
    }

    if (registry.lights.Has(originalEntityId)) {
        registry.lights.Add(entityId);
    }

    if (registry.nameTags.Has(originalEntityId))
    {
        auto& tag = registry.nameTags.Get(originalEntityId);
        std::string newName = GenerateDuplicateName(this, tag.name);
        registry.nameTags.Add(entityId, NameTagComponent(newName));
    }

    if (registry.skeletons.Has(originalEntityId)) {
        auto& sk = registry.skeletons.Get(originalEntityId);
        registry.skeletons.Add(entityId, SkeletonComponent(sk.skeletonID));
    }

    if (registry.animations.Has(originalEntityId)) {
        AnimationComponent& anim = registry.animations.Get(originalEntityId);
        registry.animations.Add(entityId, AnimationComponent(anim.animationIDs));
    }

    // ----- COLLIDER ----
    //if (registry.colliders.Has(originalEntityId)) {
    //    ColliderComponent& col = registry.colliders.Get(originalEntityId);    
    //    registry.colliders.Add(entityId, ColliderComponent(col.shapes));
    //}

    entities.push_back(std::move(entity));
    rootEntities.push_back(entityId);
    return entities.back().get();
}

Entity Scene::GetRootParent(const Entity& entityID)
{
    Entity currentID = entityID;

    while (registry.hierarchies.Has(currentID))
    {
        auto& h = registry.hierarchies.Get(currentID);

        if (h.parent == NullEntity)
            break;

        currentID = h.parent;
    }

    return currentID;
}

void Scene::RemoveEntity(const Entity id)
{
    if (registry.hierarchies.Has(id))
    {
        auto& h = registry.hierarchies.Get(id);

        for (Entity child : h.children)
        {
            if (registry.hierarchies.Has(child))
            {
                registry.hierarchies.Get(child).parent = NullEntity;
                rootEntities.push_back(child);
            }
        }

        if (h.parent != NullEntity && registry.hierarchies.Has(h.parent))
        {
            auto& parentH = registry.hierarchies.Get(h.parent);
            parentH.children.erase(
                std::remove(parentH.children.begin(),
                    parentH.children.end(),
                    id),
                parentH.children.end()
            );
        }

        registry.hierarchies.Remove(id);
    }

    rootEntities.erase(
        std::remove(rootEntities.begin(), rootEntities.end(), id),
        rootEntities.end()
    );

    entities.erase(
        std::remove_if(
            entities.begin(),
            entities.end(),
            [&](const std::unique_ptr<Entity>& e)
            {
                return *e == id;
            }
        ),
        entities.end()
    );

    if (registry.transforms.Has(id))    registry.transforms.Remove(id);
    if (registry.meshFilters.Has(id))   registry.meshFilters.Remove(id);
    if (registry.meshRenderers.Has(id)) registry.meshRenderers.Remove(id);
    if (registry.lights.Has(id))        registry.lights.Remove(id);
    if (registry.nameTags.Has(id))      registry.nameTags.Remove(id);
    if (registry.animations.Has(id))    registry.animations.Remove(id);
    if (registry.cameras.Has(id))       registry.cameras.Remove(id);
}

void Scene::RemoveEntityRecursive(Entity id)
{
    if (registry.hierarchies.Has(id))
    {
        auto children = registry.hierarchies.Get(id).children;
        for (Entity child : children)
            RemoveEntityRecursive(child);
    }

    RemoveEntity(id);
}

const Entity* Scene::getEntityByID(const Entity& id) const {
    for (auto& entity : entities) {
        if (*entity == id) {
            return entity.get();
        }
    }
    return nullptr;
}

Entity* Scene::getEntityByID(const Entity& id) {
    if (id == NullEntity)
        return nullptr;

    for (auto& entity : entities) {
        if (*entity == id) {
            return entity.get();
        }
    }
    return nullptr;
}

bool Scene::HasChildren(Entity entityID) const
{
    if (!registry.hierarchies.Has(entityID))
        return false;

    return !registry.hierarchies.Get(entityID).children.empty();
}

const std::vector<Entity>& Scene::GetChildren(Entity entityID) const
{
    static std::vector<Entity> empty;

    if (!registry.hierarchies.Has(entityID))
        return empty;

    return registry.hierarchies.Get(entityID).children;
}

void Scene::SetParent(Entity child, Entity parent)
{
    if (!registry.hierarchies.Has(child))
        registry.hierarchies.Add(child);

    if (!registry.hierarchies.Has(parent))
        registry.hierarchies.Add(parent);

    auto& childH = registry.hierarchies.Get(child);

    if (childH.parent != NullEntity)
    {
        auto& oldParentH = registry.hierarchies.Get(childH.parent);
        std20::erase(oldParentH.children, child);
    }
    else
    {
        std20::erase(rootEntities, child);
    }

    childH.parent = parent;
    registry.hierarchies.Get(parent).children.push_back(child);

    if (registry.transforms.Has(child))
    {
        auto& t = registry.transforms.Get(child);
        t.worldDirty = true;
    }
}

void Scene::MakeOrphan(Entity child)
{
    if (!registry.hierarchies.Has(child))
        return;

    auto& h = registry.hierarchies.Get(child);

    if (h.parent != NullEntity)
    {
        auto& parentH = registry.hierarchies.Get(h.parent);
        std20::erase(parentH.children, child);
    }

    h.parent = NullEntity;
    rootEntities.push_back(child);

    if (registry.transforms.Has(child))
        registry.transforms.Get(child).localDirty = true;
}

std::string Scene::GenerateDuplicateName(Scene* scene, const std::string& baseName)
{
    int counter = 1;
    std::string newName;

    while (true)
    {
        newName = baseName + "_" + std::to_string(counter);

        bool exists = false;

        for (auto& e : scene->getEntities())
        {
            if (!scene->GetRegistry().nameTags.Has(*e))
                continue;

            auto& tag = scene->GetRegistry().nameTags.Get(*e);

            if (tag.name == newName)
            {
                exists = true;
                break;
            }
        }

        if (!exists)
            return newName;

        counter++;
    }
}

std::unique_ptr<Scene> Scene::Clone()
{
    auto newScene = std::make_unique<Scene>(name + "_runtime", UUID());

    std::unordered_map<Entity, Entity> entityMap;

    for (const auto& e : entities)
    {
        Entity newID = nextEntityID++;

        entityMap[*e] = newID;

        newScene->entities.push_back(
            std::make_unique<Entity>(newID)
        );
    }

    for (Entity root : rootEntities)
    {
        newScene->rootEntities.push_back(
            entityMap[root]
        );
    }

    Registry& newReg = newScene->GetRegistry();
    const Registry& thisReg = registry;

    newReg.hierarchies.Clear();

    const auto& hierDense = thisReg.hierarchies.GetDense();
    const auto& hierEntities = thisReg.hierarchies.GetEntities();

    for (size_t i = 0; i < hierDense.size(); ++i)
    {
        const HierarchyComponent& oldComp = hierDense[i];
        const Entity               oldEntity = hierEntities[i];

        auto entityIt = entityMap.find(oldEntity);
        if (entityIt == entityMap.end())
            continue;

        Entity newEntity = entityIt->second;
        HierarchyComponent newComp;

        if (oldComp.parent != NullEntity)
        {
            auto parentIt = entityMap.find(oldComp.parent);
            newComp.parent = (parentIt != entityMap.end()) ? parentIt->second : NullEntity;
        }
        else
        {
            newComp.parent = NullEntity;
        }

        newComp.children.reserve(oldComp.children.size());

        for (const Entity& oldChild : oldComp.children)
        {
            auto childIt = entityMap.find(oldChild);
            if (childIt != entityMap.end())
                newComp.children.push_back(childIt->second);
        }

        newReg.hierarchies.Add(newEntity, std::move(newComp));
    }

    newReg.transforms.CloneFrom(thisReg.transforms, entityMap);
    newReg.meshFilters.CloneFrom(thisReg.meshFilters, entityMap);
    newReg.meshRenderers.CloneFrom(thisReg.meshRenderers, entityMap);
    newReg.skeletons.CloneFrom(thisReg.skeletons, entityMap);
    newReg.animations.CloneFrom(thisReg.animations, entityMap);
    newReg.cameras.CloneFrom(thisReg.cameras, entityMap);
    newReg.rigidBodies.CloneFrom(thisReg.rigidBodies, entityMap);
    newReg.colliders.CloneFrom(thisReg.colliders, entityMap);
    newReg.hierarchies.CloneFrom(thisReg.hierarchies, entityMap);
    newReg.lights.CloneFrom(thisReg.lights, entityMap);

    Entity oldPrimary = primaryCamera;
    if (oldPrimary != NullEntity)
    {
        auto it = entityMap.find(oldPrimary);
        if (it != entityMap.end())
            newScene->SetPrimaryCamera(it->second);
    }

    Entity oldDirectionalShadowCaster = directionalShadowCaster;
    if (oldDirectionalShadowCaster != NullEntity) {
        auto it = entityMap.find(oldDirectionalShadowCaster);
        if (it != entityMap.end())
            newScene->SetDirectionalShadowCaster(it->second);
    }

    Entity oldPointShadowCaster = pointShadowCaster;
    if (oldPointShadowCaster != NullEntity) {
        auto it = entityMap.find(oldPointShadowCaster);
        if (it != entityMap.end())
            newScene->SetPointShadowCaster(it->second);
    }

    return newScene;
}
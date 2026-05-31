#include "Scene.h"
#include "utils/C++20.h"
#include <iostream>

using namespace Lengine;

Entity Scene::createEntity_root(const std::string& name)
{
    Entity id = nextEntityID++;

    registry.CreateEntity();
    registry.nameTags.Add(id, NameTagComponent(name));

    rootEntities.push_back(id);

    return id;
}

Entity Scene::createEntity(const std::string& name)
{
    Entity id = nextEntityID++;

    registry.CreateEntity();
    registry.nameTags.Add(id, NameTagComponent(name));

    return id;
}

Entity Scene::DuplicateEntityRecursive(Entity originalID, Entity newParent, Entity newRoot)
{
    Entity newEntity = addEntity(nextEntityID++, originalID);

    if (newRoot == NullEntity)
        newRoot = newEntity;

    if (newParent != NullEntity)
    {
        auto& h = registry.hierarchies.Add(newEntity);
        h.parent = newParent;
        registry.hierarchies.Get(newParent).children.push_back(newEntity);

        std20::erase(rootEntities, newEntity);
    }
    else
    {
        registry.hierarchies.Add(newEntity);
    }

    if (registry.meshFilters.Has(newEntity))
    {
        registry.meshFilters.Get(newEntity).rootParent = newRoot;
    }

    if (registry.hierarchies.Has(originalID))
    {
        const auto& children = registry.hierarchies.Get(originalID).children;

        for (Entity child : children)
        {
            DuplicateEntityRecursive(child, newEntity, newRoot);
        }
    }

    return newEntity;
}

Entity Scene::DuplicateHierarchy(Entity rootID)
{
    return DuplicateEntityRecursive(rootID, NullEntity, NullEntity);
}

Entity Scene::addEntity(Entity entityId, const Entity originalEntityId)
{
    if (entityId == NullEntity)
        entityId = nextEntityID++;

    if (registry.transforms.Has(originalEntityId)) {
        const TransformComponent& oldTrans = registry.transforms.Get(originalEntityId);
        registry.transforms.Add(entityId, TransformComponent(oldTrans));

        auto& t = registry.transforms.Get(entityId);
        glm::vec3 offset = glm::vec3{ (t.GetWorldScale().x * 1.0f), 0.0f, (t.GetWorldScale().z * 1.0f) };
        t.localPosition += offset;
        t.localDirty = true;
        t.worldDirty = true;
    }

    if (registry.meshFilters.Has(originalEntityId)) {
        const MeshFilter& oldMf = registry.meshFilters.Get(originalEntityId);
        registry.meshFilters.Add(entityId, MeshFilter(oldMf.meshID, entityId));

        std::cout << " Scene.cpp :: root: " << GetRootParent(entityId) << std::endl;
    }

    if (registry.meshRenderers.Has(originalEntityId)) {
        const MeshRenderer& oldMr = registry.meshRenderers.Get(originalEntityId);
        registry.meshRenderers.Add(entityId, MeshRenderer(oldMr));
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

    registry.GetEntities().push_back(entityId);
    rootEntities.push_back(entityId);

    return entityId;
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
                std::remove(parentH.children.begin(), parentH.children.end(), id),
                parentH.children.end()
            );
        }

        registry.hierarchies.Remove(id);
    }

    std20::erase(rootEntities, id);
    std20::erase(registry.GetEntities(), id);

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

        for (const Entity& e : scene->GetRegistry().GetEntities())
        {
            if (!scene->GetRegistry().nameTags.Has(e))
                continue;

            auto& tag = scene->GetRegistry().nameTags.Get(e);

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

    for (const Entity& e : registry.GetEntities())
    {
        entityMap[e] = e;
        newScene->GetRegistry().GetEntities().push_back(e);
    }

    for (Entity root : rootEntities)
        newScene->rootEntities.push_back(entityMap[root]);

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

    newReg.nameTags.CloneFrom(thisReg.nameTags, entityMap);
    newReg.transforms.CloneFrom(thisReg.transforms, entityMap);
    newReg.meshFilters.CloneFrom(thisReg.meshFilters, entityMap);
    newReg.meshRenderers.CloneFrom(thisReg.meshRenderers, entityMap);
    newReg.skeletons.CloneFrom(thisReg.skeletons, entityMap);
    newReg.animations.CloneFrom(thisReg.animations, entityMap);
    newReg.cameras.CloneFrom(thisReg.cameras, entityMap);
    newReg.rigidBodies.CloneFrom(thisReg.rigidBodies, entityMap);
    newReg.colliders.CloneFrom(thisReg.colliders, entityMap);
    newReg.lights.CloneFrom(thisReg.lights, entityMap);
    newReg.controllers.CloneFrom(thisReg.controllers, entityMap);
    newReg.movements.CloneFrom(thisReg.movements, entityMap);
    newReg.scripts.CloneFrom(thisReg.scripts, entityMap);

    if (primaryCamera != NullEntity)
    {
        auto it = entityMap.find(primaryCamera);
        if (it != entityMap.end())
            newScene->SetPrimaryCamera(it->second);
    }

    if (directionalShadowCaster != NullEntity)
    {
        auto it = entityMap.find(directionalShadowCaster);
        if (it != entityMap.end())
            newScene->SetDirectionalShadowCaster(it->second);
    }

    if (pointShadowCaster != NullEntity)
    {
        auto it = entityMap.find(pointShadowCaster);
        if (it != entityMap.end())
            newScene->SetPointShadowCaster(it->second);
    }

    for (auto& mf : newReg.meshFilters.GetDense())
    {
        if (mf.rootParent != NullEntity)
        {
            auto it = entityMap.find(mf.rootParent);
            mf.rootParent = (it != entityMap.end()) ? it->second : NullEntity;
        }
    }

    for (auto& col : newReg.colliders.GetDense()) {
        for (auto& shape : col.shapes)
        {
            shape.runtimeShape = nullptr;
            shape.dirty = true;
        }
    }

    return newScene;
}
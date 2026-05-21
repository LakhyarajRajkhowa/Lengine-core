#include "TransformSystem.h"

using namespace Lengine;

bool TransformSystem::Dirty = true;

void TransformSystem::Update(
    ComponentStorage<TransformComponent>& transforms,
    const ComponentStorage<HierarchyComponent>& hierarchys,
	const std::vector<UUID> rootEntities
) {
    if (TransformSystem::Dirty) {
        const glm::mat4 identity(1.0f);

        for (UUID root : rootEntities)
        {
            UpdateWorldTransformRecursive(root, identity, true, transforms, hierarchys);
        }

        TransformSystem::Dirty = false;
    }
}

void TransformSystem::UpdateWorldTransformRecursive(
    UUID entityID,
    const glm::mat4& parentWorld,
    bool parentWorldDirty,
    ComponentStorage<TransformComponent>& transforms,
    const ComponentStorage<HierarchyComponent>& hierarchys
) {
   
    if (!transforms.Has(entityID)) return;

    auto& t = transforms.Get(entityID);

    if (t.localDirty)
    {
        TransformSystem::RecalculateLocalMatrix(t);
        t.localDirty = false;
    }

    bool worldDirty = parentWorldDirty || t.worldDirty;

    if (worldDirty)
    {
        t.worldMatrix = parentWorld * t.localMatrix;
        t.worldDirty = false;
    }

    if (hierarchys.Has(entityID)) {
        const auto& h = hierarchys.Get(entityID);
        for (UUID child : h.children)
            UpdateWorldTransformRecursive(child, t.worldMatrix, worldDirty, transforms, hierarchys);
        } 

}

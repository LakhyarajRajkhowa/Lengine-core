// PhysXManager.cpp
#include "PhysicsSystem.h"
#include <iostream>

using namespace Lengine;

bool PhysicsSystem::dirty = true;


void PhysicsSystem::Init() {
    static PxDefaultAllocator gAllocator;
    static PxDefaultErrorCallback gErrorCallback;

   
    foundation = PxCreateFoundation(PX_PHYSICS_VERSION, gAllocator, gErrorCallback);

    if (!foundation)
    {
        std::cout << "PhysX foundation creation failed\n";
        return;
    }

    gPvd = PxCreatePvd(*foundation);

    PxPvdTransport* transport =
        PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);

    gPvd->connect(*transport, PxPvdInstrumentationFlag::eALL);

    physics = PxCreatePhysics(
        PX_PHYSICS_VERSION,
        *foundation,
        PxTolerancesScale(),
        true,
        gPvd
    );

    if (!physics)
    {
        std::cout << "PhysX creation failed\n";
        return;
    }

    material = physics->createMaterial(0.5f, 0.5f, 0.6f);

    PxSceneDesc sceneDesc(physics->getTolerancesScale());
    sceneDesc.gravity = PxVec3(0, -9.81f, 0);
    dispatcher = PxDefaultCpuDispatcherCreate(2);
    sceneDesc.cpuDispatcher = dispatcher;
    sceneDesc.filterShader = PxDefaultSimulationFilterShader;

    scene = physics->createScene(sceneDesc);

    CreateGroundPlane();


}
void PhysicsSystem::InitForRuntime(Scene& scene)
{
    // Clear all editor actors from PhysX scene and the map
    for (auto& [entity, actor] : actors)
        if (actor->actor)
            this->scene->removeActor(*actor->actor);
    actors.clear();

    Registry& reg = scene.GetRegistry();
    auto& colEntities = reg.colliders.GetEntities();
    auto& colDense = reg.colliders.GetDense();

    for (size_t i = 0; i < colDense.size(); ++i)
    {
        Entity entity = colEntities[i];
        ColliderComponent& col = colDense[i];  // mutable ref — we'll fill runtimeShape

        for (auto& shape : col.shapes)
            AddCollider(entity, col, shape.type); // creates PxActor + PxShape fresh
    }

    auto& rbEntities = reg.rigidBodies.GetEntities();
    auto& rbDense = reg.rigidBodies.GetDense();

    for (size_t i = 0; i < rbDense.size(); ++i)
    {
        Entity entity = rbEntities[i];
        RigidbodyComponent& rb = const_cast<RigidbodyComponent&>(rbDense[i]);
        AddRigidbody(entity, rb);
    }

    // Sync initial positions from scene transforms into PhysX
    SyncTransformsToPhysX(reg.transforms);
}

void PhysicsSystem::SyncTransformsToPhysX(ComponentStorage<TransformComponent>& transforms)
{
    for (auto& [entity, physActor] : actors)
    {
        if (!transforms.Has(entity)) continue;
        auto& t = transforms.Get(entity);
        glm::vec3 pos = t.GetWorldPosition();
        PxTransform pose(PxVec3(pos.x, pos.y, pos.z));
        physActor->actor->setGlobalPose(pose);
    }
}

void PhysicsSystem::update(
    float dt,
    ComponentStorage<TransformComponent>& transforms
) {

    scene->simulate(dt);
    scene->fetchResults(true);

    UpdateTransforms(transforms);

}

void PhysicsSystem::UpdateTransforms(ComponentStorage<TransformComponent>& transforms) {
    for (auto& [entity, physicsActor] : actors)
    {
        if (!physicsActor->actor)
            continue;

      
        if (!transforms.Has(entity))
            continue;

        auto& transform = transforms.Get(entity);

        

        if (physicsActor->type == PhysicsActor::Type::STATIC) {
            PxTransform pose(PxVec3(
                transform.GetWorldPosition().x,
                transform.GetWorldPosition().y,
                transform.GetWorldPosition().z)
            );

            physicsActor->actor->setGlobalPose(pose);
            continue;

        }

        physx::PxTransform pose = physicsActor->actor->getGlobalPose();

        transform.localPosition = {
            pose.p.x,
            pose.p.y,
            pose.p.z
        };

        transform.localRotation = {
            pose.q.x,
            pose.q.y,
            pose.q.z,
            pose.q.w
        };

        transform.localDirty = true;
        transform.worldDirty = true;
    }
    
}

void PhysicsSystem::AddCollider(
    const Entity& entity,
    ColliderComponent& collider,
    ColliderShape::Type type
)
{
    // Create new collider shape
    ColliderShape shape;
    shape.type = type;

    collider.shapes.push_back(shape);
    ColliderShape& newShape = collider.shapes.back();

    PxRigidActor* pxActor = nullptr;

    // ------------------------------------------------
    // Ensure actor exists
    // ------------------------------------------------
    if (actors.find(entity) == actors.end())
    {
        PxTransform pose(PxVec3(0, 0, 0));

        pxActor = physics->createRigidStatic(pose);
        pxActor->userData = (void*)(uint64_t)entity;

        scene->addActor(*pxActor);

        auto physActor = std::make_unique<PhysicsActor>();
        physActor->actor = pxActor;
        physActor->type = PhysicsActor::Type::STATIC;

        actors[entity] = std::move(physActor);
    }
    else
    {
        pxActor = actors[entity]->actor;
    }

    PhysicsActor* actor = actors[entity].get();

    // ------------------------------------------------
    // Create PhysX shape
    // ------------------------------------------------
    if (newShape.type == ColliderShape::Type::Box)
    {
        PxBoxGeometry geom(
            newShape.size.x * 0.5f,
            newShape.size.y * 0.5f,
            newShape.size.z * 0.5f
        );

        newShape.runtimeShape = physics->createShape(geom, *material);
    }
    else if (newShape.type == ColliderShape::Type::Sphere)
    {
        PxSphereGeometry geom(newShape.radius);

        newShape.runtimeShape = physics->createShape(geom, *material);
    }
    else if (newShape.type == ColliderShape::Type::Capsule)
    {
        PxCapsuleGeometry geom(
            newShape.radius,
            newShape.height * 0.5f
        );

        newShape.runtimeShape = physics->createShape(geom, *material);
    }

    // ------------------------------------------------
    // Attach shape to actor
    // ------------------------------------------------
    if (newShape.runtimeShape)
    {
        actor->actor->attachShape(*newShape.runtimeShape);
    }
}

void PhysicsSystem::AddRigidbody(
    const Entity& entity,
    RigidbodyComponent& rb
)
{
    PhysicsActor* physActor = nullptr;

    // Check if actor exists
    if (actors.find(entity) != actors.end())
        physActor = actors[entity].get();

    // --------------------------------------------------
    // Case 1 : No actor exists -> create dynamic actor
    // --------------------------------------------------
    if (!physActor)
    {
        PxTransform transform(PxIdentity);

        PxRigidDynamic* dynamicActor = physics->createRigidDynamic(transform);
        dynamicActor->setMass(rb.mass);

        scene->addActor(*dynamicActor);

        auto newActor = std::make_unique<PhysicsActor>();
        newActor->actor = dynamicActor;
        newActor->type = PhysicsActor::Type::DYNAMIC;

        actors[entity] = std::move(newActor);

        return;
    }

    PxRigidActor* actor = physActor->actor;

    // --------------------------------------------------
    // Case 2 : Already dynamic
    // --------------------------------------------------
    if (physActor->type == PhysicsActor::Type::DYNAMIC)
    {
        PxRigidDynamic* dynamicActor = static_cast<PxRigidDynamic*>(actor);

        dynamicActor->setMass(rb.mass);

        return;
    }

    // --------------------------------------------------
    // Case 3 : Static -> Convert to Dynamic
    // --------------------------------------------------
    if (physActor->type == PhysicsActor::Type::STATIC)
    {
        PxRigidStatic* staticActor = static_cast<PxRigidStatic*>(actor);

        PxTransform transform = staticActor->getGlobalPose();
        PxRigidDynamic* dynamicActor = physics->createRigidDynamic(transform);

        dynamicActor->setMass(rb.mass);

        // Transfer shapes
        PxShape* shapes[32];
        PxU32 shapeCount = staticActor->getNbShapes();
        staticActor->getShapes(shapes, shapeCount);

        for (PxU32 i = 0; i < shapeCount; i++)
        {
            PxShape* shape = shapes[i];

            staticActor->detachShape(*shape);
            dynamicActor->attachShape(*shape);
        }

        scene->addActor(*dynamicActor);

        scene->removeActor(*staticActor);
        staticActor->release();

        physActor->actor = dynamicActor;
        physActor->type = PhysicsActor::Type::DYNAMIC;
    }
}

void PhysicsSystem::DeleteColliderShape(
    const Entity& entity,
    ColliderComponent& collider,
    size_t shapeIndex
)
{
    if (shapeIndex >= collider.shapes.size())
        return;

    ColliderShape& shape = collider.shapes[shapeIndex];

    // Find actor
    auto it = actors.find(entity);
    if (it != actors.end())
    {
        physx::PxRigidActor* actor = it->second->actor;

        if (shape.runtimeShape)
        {
            actor->detachShape(*shape.runtimeShape);
            shape.runtimeShape->release();
            shape.runtimeShape = nullptr;
        }
    }

    // Remove shape from vector
    collider.shapes.erase(collider.shapes.begin() + shapeIndex);
}

void PhysicsSystem::DeleteCollider(
    const Entity& entity,
    ColliderComponent& collider
)
{
    auto it = actors.find(entity);
    if (it == actors.end())
        return;

    physx::PxRigidActor* actor = it->second->actor;

    // Release all shapes
    for (auto& shape : collider.shapes)
    {
        if (shape.runtimeShape)
        {
            actor->detachShape(*shape.runtimeShape);
            shape.runtimeShape->release();
            shape.runtimeShape = nullptr;
        }
    }

    collider.shapes.clear();

    // If actor is static -> delete actor
    if (actor->is<physx::PxRigidStatic>())
    {
        scene->removeActor(*actor);
        actor->release();
        actors.erase(entity);
    }

    // If actor is dynamic -> do nothing
}

void PhysicsSystem::DeleteRigidBody(
    const Entity& entity,
    ComponentStorage<ColliderComponent>& colliders
)
{
    auto it = actors.find(entity);
    if (it == actors.end())
        return;

    physx::PxRigidActor* actor = it->second->actor;

    // Actor must be dynamic
    physx::PxRigidDynamic* dynamicActor = actor->is<physx::PxRigidDynamic>();
    if (!dynamicActor)
        return;

    // -------- Check if colliders exist --------
    if (!colliders.Has(entity))
    {
        scene->removeActor(*dynamicActor);
        dynamicActor->release();
        actors.erase(entity);
        return;
    }

    auto& col = colliders.Get(entity);

    if (col.shapes.empty())
    {
        scene->removeActor(*dynamicActor);
        dynamicActor->release();
        actors.erase(entity);
        return;
    }

    // -------- Create Static Actor --------
    physx::PxTransform transform = dynamicActor->getGlobalPose();

    physx::PxRigidStatic* staticActor =
        physics->createRigidStatic(transform);

    // -------- Move Shapes --------
    physx::PxShape* shapes[32];
    physx::PxU32 shapeCount = dynamicActor->getNbShapes();

    dynamicActor->getShapes(shapes, shapeCount);

    for (physx::PxU32 i = 0; i < shapeCount; i++)
    {
        physx::PxShape* shape = shapes[i];

        dynamicActor->detachShape(*shape);
        staticActor->attachShape(*shape);
    }

    // -------- Add static actor --------
    scene->addActor(*staticActor);

    // -------- Delete dynamic actor --------
    scene->removeActor(*dynamicActor);
    dynamicActor->release();

    actors[entity]->actor = staticActor;
    actors[entity]->type = PhysicsActor::Type::STATIC;
}

void PhysicsSystem::shutdown() {
    if (scene) scene->release();
    if (dispatcher) dispatcher->release();
    if (physics) physics->release();
    if (foundation) foundation->release();
}



void PhysicsSystem::CreateGroundPlane()
{
    // Plane: normal (0,1,0) → horizontal ground
    // distance 0 → passes through origin

    physx::PxRigidStatic* plane =
        physx::PxCreatePlane(
            *physics,
            physx::PxPlane(0, 1, 0, 0),
            *material
        );

    scene->addActor(*plane);
}


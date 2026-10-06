

#include "PhysicsSystem.h"
#include <iostream>

using namespace Lengine;

bool PhysicsSystem::dirty = true;

PxFilterFlags LengineFilterShader(PxFilterObjectAttributes attr0, PxFilterData fd0,
    PxFilterObjectAttributes attr1, PxFilterData fd1,
    PxPairFlags& pairFlags, const void*, PxU32)
{
    if (PxFilterObjectIsTrigger(attr0) || PxFilterObjectIsTrigger(attr1))
    {
        pairFlags = PxPairFlag::eTRIGGER_DEFAULT;
        return PxFilterFlag::eDEFAULT;
    }
    pairFlags = PxPairFlag::eCONTACT_DEFAULT | PxPairFlag::eNOTIFY_TOUCH_FOUND | PxPairFlag::eNOTIFY_TOUCH_LOST;
    return PxFilterFlag::eDEFAULT;
}

void PhysicsSystem::Init(Scene& scene)
{
    static PxDefaultAllocator     gAllocator;
    static PxDefaultErrorCallback gErrorCallback;

    foundation = PxCreateFoundation(PX_PHYSICS_VERSION, gAllocator, gErrorCallback);
    if (!foundation) { std::cout << "[Physics] Foundation creation failed\n"; return; }

    pvd = PxCreatePvd(*foundation);
    PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
    pvd->connect(*transport, PxPvdInstrumentationFlag::eALL);

    physics = PxCreatePhysics(PX_PHYSICS_VERSION, *foundation, PxTolerancesScale(), true, pvd);
    if (!physics) { std::cout << "[Physics] PxPhysics creation failed\n"; return; }

    material = physics->createMaterial(0.5f, 0.5f, 0.6f);

    PxSceneDesc sceneDesc(physics->getTolerancesScale());
    sceneDesc.gravity = PxVec3(0.f, -9.81f, 0.f);
    dispatcher = PxDefaultCpuDispatcherCreate(2);
    sceneDesc.cpuDispatcher = dispatcher;
    sceneDesc.filterShader = LengineFilterShader;        // was PxDefaultSimulationFilterShader
    sceneDesc.simulationEventCallback = this;            // NEW

    physxScene = physics->createScene(sceneDesc);
    createGroundPlane();
    InitForScene(scene);
}

// - clear previous scene's actors
// - build actors for colliders and rgbody components
// - sync engine's transform component to PhysX scene
// - onAdd & onRemove definition for colliders and rgbodies
void PhysicsSystem::InitForScene(Scene& scene)
{
    clearScene();

    registry = &scene.GetRegistry();

    auto& colEntities = registry->colliders.GetEntities();
    auto& colDense = registry->colliders.GetDense();
    for (size_t i = 0; i < colDense.size(); ++i)
        buildColliderActor(colEntities[i], colDense[i]);

    auto& rbEntities = registry->rigidBodies.GetEntities();
    auto& rbDense = registry->rigidBodies.GetDense();
    for (size_t i = 0; i < rbDense.size(); ++i)
        buildRigidbodyActor(rbEntities[i], rbDense[i]);

    syncTransformsToPhysX(registry->transforms);

    registry->colliders.onAdd = [this](Entity e, ColliderComponent&)
        {
            pendingColliderRemoved.erase(e);
            pendingColliderAdded.insert(e);
        };

    registry->colliders.onRemove = [this](Entity e, ColliderComponent&)
        {
            pendingColliderAdded.erase(e);
            pendingColliderRemoved.insert(e);
        };

    registry->rigidBodies.onAdd = [this](Entity e, RigidbodyComponent&)
        {
            pendingRigidbodyRemoved.erase(e);
            pendingRigidbodyAdded.insert(e);
        };

    registry->rigidBodies.onRemove = [this](Entity e, RigidbodyComponent&)
        {
            pendingRigidbodyAdded.erase(e);
            pendingRigidbodyRemoved.insert(e);
        };
}

void PhysicsSystem::clearScene()
{
    for (auto& [entity, actor] : actors)
    {
        if (!actor->actor) continue;

        PxShape* shapes[32];
        PxU32 count = actor->actor->getNbShapes();
        count = (count > 32) ? 32 : count;
        actor->actor->getShapes(shapes, count);

        for (PxU32 i = 0; i < count; ++i)
        {
            actor->actor->detachShape(*shapes[i]);
            shapes[i]->release();
        }

        // reset component-side pointers so a later buildColliderActor doesn't skip rebuilding
        if (registry && registry->colliders.Has(entity))
        {
            auto& col = registry->colliders.Get(entity);
            for (auto& shape : col.shapes)
                shape.runtimeShape = nullptr;
        }

        physxScene->removeActor(*actor->actor);
        actor->actor->release(); // also fixes the actor leak — it was never released before
    }

    actors.clear();
    pendingColliderAdded.clear();
    pendingColliderRemoved.clear();
    pendingRigidbodyAdded.clear();
    pendingRigidbodyRemoved.clear();
}

void PhysicsSystem::Shutdown()
{
    if (physxScene) physxScene->release();
    if (dispatcher) dispatcher->release();
    if (physics)    physics->release();
    if (foundation) foundation->release();
}



// - physics actors creation and destruction queues process and cleared
// - actual simulation of physics through the PhysX
// - sync PhysX transforms to engine's transform components 
void PhysicsSystem::UpdateRuntime(float dt, ComponentStorage<TransformComponent>& transforms)
{
    flushPending();
    updateKinematicBodies(dt);
    physxScene->simulate(dt);
    physxScene->fetchResults(true);
    updateTransforms(transforms);
}



void PhysicsSystem::flushPending()
{
    if (pendingColliderAdded.empty() &&
        pendingColliderRemoved.empty() &&
        pendingRigidbodyAdded.empty() &&
        pendingRigidbodyRemoved.empty())
        return;

    for (Entity e : pendingColliderRemoved)
        if (registry->colliders.Has(e))
            teardownCollider(e, registry->colliders.Get(e));
    pendingColliderRemoved.clear();

    for (Entity e : pendingRigidbodyRemoved)
    {
        ColliderComponent* col = registry->colliders.Has(e)
            ? &registry->colliders.Get(e) : nullptr;
        teardownRigidbody(e, col);
    }
    pendingRigidbodyRemoved.clear();

    for (Entity e : pendingColliderAdded)
        if (registry->colliders.Has(e))
            buildColliderActor(e, registry->colliders.Get(e));
    pendingColliderAdded.clear();

    for (Entity e : pendingRigidbodyAdded)
        if (registry->rigidBodies.Has(e))
            buildRigidbodyActor(e, registry->rigidBodies.Get(e));
    pendingRigidbodyAdded.clear();
}



void PhysicsSystem::buildColliderActor(Entity entity, ColliderComponent& collider)
{
    PxRigidActor* pxActor = nullptr;
    glm::vec3 ws(1.0f);

    if (actors.find(entity) == actors.end())
    {
        PxTransform pose(PxVec3(0.f));
        if (registry->transforms.Has(entity))
        {
            auto& t = registry->transforms.Get(entity);
            glm::vec3 pos = t.GetWorldPosition();
            glm::quat rot = t.GetWorldRotation();
            glm::vec3 ws(1.0f); // transform scale shouldnt effect collider size
            pose = PxTransform(PxVec3(pos.x, pos.y, pos.z),
                PxQuat(rot.x, rot.y, rot.z, rot.w));
        }

        pxActor = physics->createRigidStatic(pose);
        pxActor->userData = (void*)(uint64_t)entity;
        physxScene->addActor(*pxActor);

        auto pa = std::make_unique<PhysicsActor>();
        pa->actor = pxActor;
        pa->type = PhysicsActor::Type::STATIC;
        actors[entity] = std::move(pa);
    }
    else
    {
        pxActor = actors[entity]->actor;
        if (registry->transforms.Has(entity))
            ws = registry->transforms.Get(entity).GetWorldScale();
    }

    for (auto& shape : collider.shapes)
    {
        if (shape.runtimeShape) continue;

        if (shape.type == ColliderShape::Type::Box)
        {
            PxBoxGeometry geom(shape.size.x * ws.x * 0.5f,
                shape.size.y * ws.y * 0.5f,
                shape.size.z * ws.z * 0.5f);
            shape.runtimeShape = physics->createShape(geom, *material, true);
        }
        else if (shape.type == ColliderShape::Type::Sphere)
        {
            float us = glm::max(ws.x, glm::max(ws.y, ws.z));
            shape.runtimeShape = physics->createShape(PxSphereGeometry(shape.radius * us), *material, true);
        }
        else if (shape.type == ColliderShape::Type::Capsule)
        {
            float rs = glm::max(ws.y, ws.z);
            PxCapsuleGeometry geom(shape.radius * rs, shape.height * ws.x * 0.5f);
            shape.runtimeShape = physics->createShape(geom, *material, true);
        }

        if (shape.runtimeShape)
        {
            shape.runtimeShape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, !shape.isTrigger);
            shape.runtimeShape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, shape.isTrigger);
            pxActor->attachShape(*shape.runtimeShape);
        }
    }
}

void PhysicsSystem::buildRigidbodyActor(Entity entity, RigidbodyComponent& rb)
{
    auto it = actors.find(entity);

    // Case 1: no actor yet — create standalone dynamic
    if (it == actors.end())
    {
        glm::vec3 pos(0.0f);
        glm::quat rot(1, 0, 0, 0);
        if (registry->transforms.Has(entity))
        {
            auto& t = registry->transforms.Get(entity);
            pos = t.GetWorldPosition();
            rot = t.GetWorldRotation();
        }

        PxRigidDynamic* dyn = physics->createRigidDynamic(
            PxTransform(PxVec3(pos.x, pos.y, pos.z),
                PxQuat(rot.x, rot.y, rot.z, rot.w)));

        dyn->userData = (void*)(uint64_t)entity;
        syncRigidbodyProperties(dyn, rb);
        physxScene->addActor(*dyn);

        auto pa = std::make_unique<PhysicsActor>();
        pa->actor = dyn;
        pa->type = rb.isKinematic ? PhysicsActor::Type::KINEMATIC : PhysicsActor::Type::DYNAMIC;
        actors[entity] = std::move(pa);
        rb.dirty = false;
        return;
    }

    PhysicsActor* pa = it->second.get();

    // Case 2: already dynamic/kinematic — re-sync all properties
    if (pa->type == PhysicsActor::Type::DYNAMIC ||
        pa->type == PhysicsActor::Type::KINEMATIC)
    {
        syncRigidbodyProperties(static_cast<PxRigidDynamic*>(pa->actor), rb);
        pa->type = rb.isKinematic ? PhysicsActor::Type::KINEMATIC : PhysicsActor::Type::DYNAMIC;
        rb.dirty = false;
        return;
    }

    // Case 3: static exists (collider first) — promote to dynamic
    PxRigidStatic* staticActor = static_cast<PxRigidStatic*>(pa->actor);
    PxTransform     pose = staticActor->getGlobalPose();
    PxRigidDynamic* dynamicActor = physics->createRigidDynamic(pose);


    dynamicActor->userData = (void*)(uint64_t)entity;

    PxShape* shapes[32];
    PxU32 count = staticActor->getNbShapes();
    staticActor->getShapes(shapes, count);
    for (PxU32 i = 0; i < count; ++i)
    {
        staticActor->detachShape(*shapes[i]);
        dynamicActor->attachShape(*shapes[i]);
    }

    syncRigidbodyProperties(dynamicActor, rb);

    physxScene->addActor(*dynamicActor);
    physxScene->removeActor(*staticActor);
    staticActor->release();

    pa->actor = dynamicActor;
    pa->type = rb.isKinematic ? PhysicsActor::Type::KINEMATIC : PhysicsActor::Type::DYNAMIC;

    drainPendingCommands(entity, dynamicActor, rb);

    rb.dirty = false;
}



void PhysicsSystem::teardownCollider(Entity entity, ColliderComponent& collider)
{
    auto it = actors.find(entity);
    if (it == actors.end()) return;

    PxRigidActor* actor = it->second->actor;

    for (auto& shape : collider.shapes)
    {
        if (!shape.runtimeShape) continue;
        actor->detachShape(*shape.runtimeShape);
        shape.runtimeShape->release();
        shape.runtimeShape = nullptr;
    }
    collider.shapes.clear();

    if (it->second->type == PhysicsActor::Type::STATIC)
    {
        physxScene->removeActor(*actor);
        actor->release();
        actors.erase(it);
    }
}

void PhysicsSystem::teardownRigidbody(Entity entity, ColliderComponent* collider)
{
    auto it = actors.find(entity);
    if (it == actors.end()) return;

    PxRigidDynamic* dyn = it->second->actor->is<PxRigidDynamic>();
    if (!dyn) return;

    if (!collider || collider->shapes.empty())
    {
        physxScene->removeActor(*dyn);
        dyn->release();
        actors.erase(it);
        return;
    }

    // Collider still present — demote dynamic back to static
    PxTransform    pose = dyn->getGlobalPose();
    PxRigidStatic* staticActor = physics->createRigidStatic(pose);
    staticActor->userData = (void*)(uint64_t)entity;

    PxShape* shapes[32];
    PxU32 count = dyn->getNbShapes();
    dyn->getShapes(shapes, count);
    for (PxU32 i = 0; i < count; ++i)
    {
        dyn->detachShape(*shapes[i]);
        staticActor->attachShape(*shapes[i]);
    }

    physxScene->addActor(*staticActor);
    physxScene->removeActor(*dyn);
    dyn->release();

    it->second->actor = staticActor;
    it->second->type = PhysicsActor::Type::STATIC;
}



// - sync game-side transforms into PhysX 
void PhysicsSystem::syncTransformsToPhysX(ComponentStorage<TransformComponent>& transforms)
{
    for (auto& [entity, pa] : actors)
    {
        if (!transforms.Has(entity)) continue;
        auto& t = transforms.Get(entity);
        glm::vec3 pos = t.GetWorldPosition();
        glm::quat rot = t.GetWorldRotation();
        pa->actor->setGlobalPose(PxTransform(PxVec3(pos.x, pos.y, pos.z),
            PxQuat(rot.x, rot.y, rot.z, rot.w)));
    }
}

// - write PhysX poses back to ECS transforms each frame
void PhysicsSystem::updateTransforms(ComponentStorage<TransformComponent>& transforms)
{
    for (auto& [entity, pa] : actors)
    {
        if (!pa->actor || !transforms.Has(entity)) continue;
        auto& t = transforms.Get(entity);
        
        if (pa->type == PhysicsActor::Type::STATIC || registry->boneAttachments.Has(entity))
        {
            glm::vec3 pos = t.GetWorldPosition();
            glm::quat rot = t.GetWorldRotation();
            pa->actor->setGlobalPose(PxTransform(PxVec3(pos.x, pos.y, pos.z),
                PxQuat(rot.x, rot.y, rot.z, rot.w)));
            continue;
        }

        PxTransform pose = pa->actor->getGlobalPose();
        t.localPosition = { pose.p.x, pose.p.y, pose.p.z };
        t.localRotation = glm::quat(pose.q.w, pose.q.x, pose.q.y, pose.q.z);
        t.localDirty = true;
        t.worldDirty = true;
    }
}

// - update psoitions of all kinematic bodies
void PhysicsSystem::updateKinematicBodies(float dt)
{
    if (!registry) return;

    auto& entities = registry->rigidBodies.GetEntities();
    auto& dense = registry->rigidBodies.GetDense();

    for (size_t i = 0; i < dense.size(); ++i)
    {
        Entity entity = entities[i];
        auto& rb = dense[i];

        if (!rb.isKinematic)
            continue;

        if (!registry->transforms.Has(entity))
            continue;

        auto& transform = registry->transforms.Get(entity);

        glm::vec3 currentPos = transform.GetWorldPosition();
        glm::quat currentRot = transform.GetWorldRotation();

        glm::vec3 targetPos =
            currentPos +
            rb.linearVelocity * dt;

        MoveKinematic(
            entity,
            targetPos,
            currentRot
        );
    }
}



void PhysicsSystem::createGroundPlane()
{
    PxRigidStatic* plane = PxCreatePlane(*physics, PxPlane(0, 1, 0, 0), *material);
    physxScene->addActor(*plane);
}

PxRigidDynamic* PhysicsSystem::getDynamicActor(Entity e) const
{
    auto it = actors.find(e);
    if (it == actors.end()) return nullptr;
    if (it->second->type == PhysicsActor::Type::STATIC) return nullptr;
    return static_cast<PxRigidDynamic*>(it->second->actor);
}

PxForceMode::Enum PhysicsSystem::toPxForceMode(ForceMode mode)
{
    switch (mode)
    {
    case ForceMode::Force:          return PxForceMode::eFORCE;
    case ForceMode::Impulse:        return PxForceMode::eIMPULSE;
    case ForceMode::VelocityChange: return PxForceMode::eVELOCITY_CHANGE;
    case ForceMode::Acceleration:   return PxForceMode::eACCELERATION;
    default:                        return PxForceMode::eFORCE;
    }
}

// - pushes all structural fields from rb into an already-created PxRigidDynamic
void PhysicsSystem::syncRigidbodyProperties(PxRigidDynamic* actor, const RigidbodyComponent& rb)
{
    if (actor->getNbShapes() > 0)
        PxRigidBodyExt::setMassAndUpdateInertia(*actor, rb.mass);
    else
        actor->setMass(rb.mass);

    actor->setLinearDamping(rb.linearDamping);
    actor->setAngularDamping(rb.angularDamping);

    actor->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, !rb.useGravity);

    actor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, rb.isKinematic);

    actor->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_LINEAR_X, rb.lockLinearX);
    actor->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_LINEAR_Y, rb.lockLinearY);
    actor->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_LINEAR_Z, rb.lockLinearZ);
    actor->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_ANGULAR_X, rb.lockAngularX);
    actor->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y, rb.lockAngularY);
    actor->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z, rb.lockAngularZ);

    if (!rb.isKinematic)
    {
        actor->setLinearVelocity(PxVec3(rb.linearVelocity.x, rb.linearVelocity.y, rb.linearVelocity.z));
        actor->setAngularVelocity(PxVec3(rb.angularVelocity.x, rb.angularVelocity.y, rb.angularVelocity.z));
    }
    
}

void PhysicsSystem::drainPendingCommands(Entity e, PxRigidDynamic* dyn, RigidbodyComponent& rb)
{
    if (dyn->getRigidBodyFlags().isSet(PxRigidBodyFlag::eKINEMATIC)) return;


    for (auto& cmd : rb.pendingForces)
    {
        PxVec3 v(cmd.value.x, cmd.value.y, cmd.value.z);
        if (cmd.type == ForceCommand::Type::Force)
            dyn->addForce(v, toPxForceMode((ForceMode)cmd.mode));
        else
            dyn->addTorque(v, toPxForceMode((ForceMode)cmd.mode));
    }

    rb.pendingForces.clear();
}


// - build or update actors's PxShape
void PhysicsSystem::AddCollider(Entity entity, ColliderComponent& col, ColliderShape::Type type)
{
    ColliderShape shape;
    shape.type = type;
    col.shapes.push_back(shape);
    buildColliderActor(entity, col);
}

// - detach PxShape from the actor
void PhysicsSystem::DeleteColliderShape(Entity entity, ColliderComponent& col, size_t shapeIndex)
{
    if (shapeIndex >= col.shapes.size()) return;

    auto it = actors.find(entity);
    if (it != actors.end())
    {
        ColliderShape& shape = col.shapes[shapeIndex];
        if (shape.runtimeShape)
        {
            it->second->actor->detachShape(*shape.runtimeShape);
            shape.runtimeShape->release();
            shape.runtimeShape = nullptr;
        }
    }
    col.shapes.erase(col.shapes.begin() + shapeIndex);
}

// - delete all PxShapes attached to the actor
// - delete the actor if it is of type STATIC
void PhysicsSystem::DeleteCollider(Entity entity, ColliderComponent& col)
{
    teardownCollider(entity, col);
}

// - delete the actor if no collider
// - demotes actor type from DYNAMIC to STATIC if collider present
void PhysicsSystem::DeleteRigidBody(Entity entity, ColliderComponent* col)
{
    teardownRigidbody(entity, col);
}


// - set mass only for dynamic actor
// - uses  PxRigidBodyExt::setMassAndUpdateInertia
void PhysicsSystem::SetMass(Entity e, float mass)
{
    if (!registry->rigidBodies.Has(e)) return;
    auto& rb = registry->rigidBodies.Get(e);
    rb.mass = mass;
    rb.dirty = true;

    PxRigidDynamic* dyn = getDynamicActor(e);
    if (!dyn) return;
    if (dyn->getNbShapes() > 0)
        PxRigidBodyExt::setMassAndUpdateInertia(*dyn, mass);
    else
        dyn->setMass(mass);
}

// - set gravity only for dynamic actor
// - uses setActorFlag(PxActorFlag::eDISABLE_GRAVITY, !enabled)
void PhysicsSystem::SetGravityEnabled(Entity e, bool enabled)
{
    if (!registry->rigidBodies.Has(e)) return;
    auto& rb = registry->rigidBodies.Get(e);
    rb.useGravity = enabled;
    rb.dirty = true;

    PxRigidDynamic* dyn = getDynamicActor(e);
    if (dyn) dyn->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, !enabled);
}

// - set kinematics only for dynamic actor
// - set Linear velocity to PxVec3(0.f)
// - set Angular velocity to PxVec3(0.f)
// - setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, kinematic)
void PhysicsSystem::SetKinematic(Entity e, bool kinematic)
{
    if (!registry->rigidBodies.Has(e)) return;
    auto& rb = registry->rigidBodies.Get(e);
    rb.isKinematic = kinematic;
    rb.dirty = true;

    PxRigidDynamic* dyn = getDynamicActor(e);
    if (!dyn) return;

    if (kinematic)
    {
        dyn->setLinearVelocity(PxVec3(0.f));
        dyn->setAngularVelocity(PxVec3(0.f));
    }
    dyn->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, kinematic);

    auto it = actors.find(e);
    if (it != actors.end())
        it->second->type = kinematic ? PhysicsActor::Type::KINEMATIC : PhysicsActor::Type::DYNAMIC;
}


// - set linear damping only for dynamic actor
// - uses  setLinearDamping(damping)
void PhysicsSystem::SetLinearDamping(Entity e, float damping)
{
    if (!registry->rigidBodies.Has(e)) return;
    auto& rb = registry->rigidBodies.Get(e);
    rb.linearDamping = damping;
    rb.dirty = true;

    PxRigidDynamic* dyn = getDynamicActor(e);
    if (dyn) dyn->setLinearDamping(damping);
}

// - set angular damping only for dynamic actor
// - uses  setAngularDamping(damping)
void PhysicsSystem::SetAngularDamping(Entity e, float damping)
{
    if (!registry->rigidBodies.Has(e)) return;
    auto& rb = registry->rigidBodies.Get(e);
    rb.angularDamping = damping;
    rb.dirty = true;

    PxRigidDynamic* dyn = getDynamicActor(e);
    if (dyn) dyn->setAngularDamping(damping);
}

void PhysicsSystem::SetLinearLock(Entity e, bool x, bool y, bool z)
{
    if (!registry->rigidBodies.Has(e)) return;
    auto& rb = registry->rigidBodies.Get(e);
    rb.lockLinearX = x; rb.lockLinearY = y; rb.lockLinearZ = z;
    rb.dirty = true;

    PxRigidDynamic* dyn = getDynamicActor(e);
    if (!dyn) return;
    dyn->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_LINEAR_X, x);
    dyn->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_LINEAR_Y, y);
    dyn->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_LINEAR_Z, z);
}

void PhysicsSystem::SetAngularLock(Entity e, bool x, bool y, bool z)
{
    if (!registry->rigidBodies.Has(e)) return;
    auto& rb = registry->rigidBodies.Get(e);
    rb.lockAngularX = x; rb.lockAngularY = y; rb.lockAngularZ = z;
    rb.dirty = true;

    PxRigidDynamic* dyn = getDynamicActor(e);
    if (!dyn) return;
    dyn->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_ANGULAR_X, x);
    dyn->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y, y);
    dyn->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z, z);
}


void PhysicsSystem::SetLinearVelocity(Entity e, glm::vec3 v)
{
    if (!registry->rigidBodies.Has(e))
        return;

    auto& rb = registry->rigidBodies.Get(e);

    rb.linearVelocity = v;
    rb.dirty = true;

    PxRigidDynamic* dyn = getDynamicActor(e);
    if (!dyn)
        return;

    if (!rb.isKinematic)
        dyn->setLinearVelocity(PxVec3(v.x, v.y, v.z));
}

void PhysicsSystem::SetAngularVelocity(Entity e, glm::vec3 v)
{
    if (!registry->rigidBodies.Has(e)) return;
    auto& rb = registry->rigidBodies.Get(e);
    rb.angularVelocity = v;
    rb.dirty = true;

    PxRigidDynamic* dyn = getDynamicActor(e);
    if (!dyn) return;
    if (dyn->getRigidBodyFlags().isSet(PxRigidBodyFlag::eKINEMATIC)) return;
    dyn->setAngularVelocity(PxVec3(v.x, v.y, v.z));
}

glm::vec3 PhysicsSystem::GetLinearVelocity(Entity e) const
{
    if (!registry->rigidBodies.Has(e))
        return glm::vec3(0.0f);

    const auto& rb = registry->rigidBodies.Get(e);

    if (rb.isKinematic)
        return rb.linearVelocity;

    PxRigidDynamic* dyn = getDynamicActor(e);
    if (!dyn)
        return glm::vec3(0.0f);

    PxVec3 v = dyn->getLinearVelocity();
    return { v.x, v.y, v.z };
}

glm::vec3 PhysicsSystem::GetAngularVelocity(Entity e) const
{
    PxRigidDynamic* dyn = getDynamicActor(e);
    if (!dyn) return glm::vec3(0.0f);
    PxVec3 v = dyn->getAngularVelocity();
    return { v.x, v.y, v.z };
}


// KINEMATIC BODY

void PhysicsSystem::MoveKinematic(Entity e,
    const glm::vec3& pos,
    const glm::quat& rot)
{
    PxRigidDynamic* dyn = getDynamicActor(e);

    if (!dyn) return;
    if (!dyn->getRigidBodyFlags().isSet(PxRigidBodyFlag::eKINEMATIC))
        return;

    dyn->setKinematicTarget(
        PxTransform(
            PxVec3(pos.x, pos.y, pos.z),
            PxQuat(rot.x, rot.y, rot.z, rot.w)
        )
    );
}

// =============================================================================
//  Force API
// =============================================================================

void PhysicsSystem::AddForce(Entity e, glm::vec3 force, ForceMode mode)
{
    PxRigidDynamic* dyn = getDynamicActor(e);
    if (!dyn)
    {
        if (registry->rigidBodies.Has(e))
            registry->rigidBodies.Get(e).pendingForces.push_back(
                { force, ForceCommand::Type::Force, (int)mode });
        return;
    }
    if (dyn->getRigidBodyFlags().isSet(PxRigidBodyFlag::eKINEMATIC)) return;
    dyn->addForce(PxVec3(force.x, force.y, force.z), toPxForceMode(mode));
}

void PhysicsSystem::AddTorque(Entity e, glm::vec3 torque, ForceMode mode)
{
    PxRigidDynamic* dyn = getDynamicActor(e);
    if (!dyn)
    {
        if (registry->rigidBodies.Has(e))
            registry->rigidBodies.Get(e).pendingForces.push_back(
                { torque, ForceCommand::Type::Torque, (int)mode });
        return;
    }
    if (dyn->getRigidBodyFlags().isSet(PxRigidBodyFlag::eKINEMATIC)) return;
    dyn->addTorque(PxVec3(torque.x, torque.y, torque.z), toPxForceMode(mode));
}

void PhysicsSystem::ClearForces(Entity e)
{
    PxRigidDynamic* dyn = getDynamicActor(e);
    if (!dyn) return;
    dyn->clearForce(PxForceMode::eFORCE);
    dyn->clearTorque(PxForceMode::eFORCE);
}


// =============================================================================
//  Sleep API
// =============================================================================

void PhysicsSystem::WakeUp(Entity e)
{
    PxRigidDynamic* dyn = getDynamicActor(e);
    if (dyn) dyn->wakeUp();
}

void PhysicsSystem::PutToSleep(Entity e)
{
    PxRigidDynamic* dyn = getDynamicActor(e);
    if (dyn) dyn->putToSleep();
}

bool PhysicsSystem::IsSleeping(Entity e) const
{
    PxRigidDynamic* dyn = getDynamicActor(e);
    if (!dyn) return true;
    return dyn->isSleeping();
}

void PhysicsSystem::SetColliderTrigger(ColliderShape& shape, bool isTrigger)
{
    if (!shape.runtimeShape) return;

    if (isTrigger)
    {
        shape.runtimeShape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, false);
        shape.runtimeShape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, true);
    }
    else
    {
        shape.runtimeShape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, false);
        shape.runtimeShape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, true);
    }
}

void PhysicsSystem::onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs)
{

    if (pairHeader.flags & (PxContactPairHeaderFlag::eREMOVED_ACTOR_0 | PxContactPairHeaderFlag::eREMOVED_ACTOR_1))
        return;

    Entity a = (Entity)(uint64_t)pairHeader.actors[0]->userData;
    Entity b = (Entity)(uint64_t)pairHeader.actors[1]->userData;

    for (PxU32 i = 0; i < nbPairs; ++i)
    {

        const PxContactPair& cp = pairs[i];
        if (cp.events & PxPairFlag::eNOTIFY_TOUCH_FOUND)
            pendingCollisionEnter.push_back({ a, b });
        else if (cp.events & PxPairFlag::eNOTIFY_TOUCH_LOST)
            pendingCollisionExit.push_back({ a, b });
    }
}

void PhysicsSystem::onTrigger(PxTriggerPair* pairs, PxU32 count)
{
    for (PxU32 i = 0; i < count; ++i)
    {
        const PxTriggerPair& p = pairs[i];
        if (p.flags & (PxTriggerPairFlag::eREMOVED_SHAPE_TRIGGER | PxTriggerPairFlag::eREMOVED_SHAPE_OTHER))
            continue;

        Entity triggerEntity = (Entity)(uint64_t)p.triggerActor->userData;
        Entity otherEntity = (Entity)(uint64_t)p.otherActor->userData;

        if (p.status == PxPairFlag::eNOTIFY_TOUCH_FOUND)
            pendingTriggerEnter.push_back({ triggerEntity, otherEntity });
        else if (p.status == PxPairFlag::eNOTIFY_TOUCH_LOST)
            pendingTriggerExit.push_back({ triggerEntity, otherEntity });
    }
}

std::vector<PhysicsSystem::CollisionEvent> PhysicsSystem::ConsumeCollisionEnterEvents()
{
    std::vector<CollisionEvent> events = std::move(pendingCollisionEnter);
    pendingCollisionEnter.clear();
    return events;
}

std::vector<PhysicsSystem::CollisionEvent> PhysicsSystem::ConsumeCollisionExitEvents()
{
    std::vector<CollisionEvent> events = std::move(pendingCollisionExit);
    pendingCollisionExit.clear();
    return events;
}

std::vector<PhysicsSystem::TriggerEvent> PhysicsSystem::ConsumeTriggerEnterEvents()
{
    std::vector<TriggerEvent> events = std::move(pendingTriggerEnter);
    pendingTriggerEnter.clear();
    return events;
}

std::vector<PhysicsSystem::TriggerEvent> PhysicsSystem::ConsumeTriggerExitEvents()
{
    std::vector<TriggerEvent> events = std::move(pendingTriggerExit);
    pendingTriggerExit.clear();
    return events;
}
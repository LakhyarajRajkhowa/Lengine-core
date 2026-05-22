// PhysXManager.h
#pragma once
#include <PxPhysicsAPI.h>

using namespace physx;


#include "transform/TransformSystem.h"



namespace Lengine {

   

    struct PhysicsActor
    {
        PxRigidActor* actor = nullptr;

        enum class Type
        {
            STATIC = 0,
            DYNAMIC = 1,
            KINEMATIC = 2
        };

        Type type = Type::STATIC;
    };

    class PhysicsSystem {
    public:

        static bool dirty;

        PhysicsSystem() = default;
        ~PhysicsSystem() = default;
       

        static PhysicsSystem& getInstance();

        void Init();
        void update(
            float dt,
            ComponentStorage<TransformComponent>& transforms
            );
        void shutdown();

        PxPhysics* getPhysics() { return physics; }
        PxScene* getScene() { return scene; }
        PxMaterial* getDefaultMaterial() { return material; }

        void PhysicsSystem::RemoveActor_collider(
            const Entity& entity,
            ComponentStorage<ColliderComponent>& colliders,
            ComponentStorage<RigidbodyComponent>& rigidbodies
        );

        void PhysicsSystem::RemoveActor_rigidbody(
            const Entity& entity,
            ComponentStorage<ColliderComponent>& colliders,
            ComponentStorage<RigidbodyComponent>& rigidbodies
        );

        void AddCollider(
            const Entity& entity,
            ColliderComponent& collider,
            ColliderShape::Type type = ColliderShape::Type::Box
        );

        void AddRigidbody(
            const Entity& entity,
            RigidbodyComponent& rb
        );

        void PhysicsSystem::DeleteColliderShape(
            const Entity& entity,
            ColliderComponent& collider,
            size_t shapeIndex
        );

        void PhysicsSystem::DeleteCollider(
            const Entity& entity,
            ColliderComponent& collider
        );

        void PhysicsSystem::DeleteRigidBody(
            const Entity& entity,
            ComponentStorage<ColliderComponent>& colliders
            );

        std::unordered_map<Entity, std::unique_ptr<PhysicsActor>>&  GetActors() { return actors; }
        const std::unordered_map<Entity, std::unique_ptr<PhysicsActor>>& GetActors() const { return actors; }

    private:


        PxFoundation* foundation = nullptr;
        PxPhysics* physics = nullptr;
        PxDefaultCpuDispatcher* dispatcher = nullptr;
        PxScene* scene = nullptr;
        PxMaterial* material = nullptr;
        PxPvd* gPvd = NULL;

        std::unordered_map<Entity, std::unique_ptr<PhysicsActor>> actors;

        void UpdateTransforms(ComponentStorage<TransformComponent>& transforms);
        void CreateGroundPlane();


    };

    inline glm::mat4 PxToGLM(const physx::PxTransform& t)
    {
        physx::PxMat44 m(t);

        return glm::mat4(
            m.column0.x, m.column0.y, m.column0.z, m.column0.w,
            m.column1.x, m.column1.y, m.column1.z, m.column1.w,
            m.column2.x, m.column2.y, m.column2.z, m.column2.w,
            m.column3.x, m.column3.y, m.column3.z, m.column3.w
        );
    }
}

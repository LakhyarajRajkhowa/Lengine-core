#pragma once

#include "scene/components/ComponentStorage.h"

#include "resources/AssetManager.h"

namespace Lengine
{

    class AnimationSystem
    {
    public:
        AnimationSystem(AssetManager& assetManager) : assetManager(assetManager) {}

        void Update(
            ComponentStorage<AnimationComponent>& animations,
            ComponentStorage<SkeletonComponent>& skeletons,
            float dt
        );

    private:
        AssetManager& assetManager;

        void ApplyAnimation(
            ComponentStorage<SkeletonComponent>& skeletons,
            UUID entity,
            AnimationComponent& anim,
            float time
        );

        void ComputeBoneTransforms(
            Skeleton& skeleton,
            Animation& animation,
            float time,
            std::vector<glm::mat4>& boneMatrices
        );

        glm::vec3 InterpolatePosition(AnimationTrack& track, float time, int delta);
        glm::quat InterpolateRotation(AnimationTrack& track, float time, int delta);
        glm::vec3 InterpolateScale(AnimationTrack& track, float time, int delta);





    };

}
#include "AnimationSystem.h"
#include <cmath>

namespace Lengine
{

    // TODO : Optimise using SoA and parallel animation

    void AnimationSystem::Update(
        ComponentStorage<AnimationComponent>& animComponents,
        ComponentStorage<SkeletonComponent>& skeletons,
        float dt
    )
    {
        for (auto& [entity, anim] : animComponents.All())
        {
            if (anim.currentAnimationID == UUID::Null)
                continue;

            Animation* animation =
                assetManager.GetAnimation(anim.currentAnimationID);

            if (!animation)
                continue;

            // Advance animation time
            anim.currentTime +=
                dt *
                animation->ticksPerSecond *
                anim.playbackSpeed;

            if (anim.looping)
            {
                anim.currentTime =
                    fmod(anim.currentTime, animation->duration);
            }
            else
            {
                anim.currentTime =
                    std::min(anim.currentTime, animation->duration);
            }

            ApplyAnimation(
                skeletons,
                entity,
                anim,
                anim.currentTime);
        }
    }

    void AnimationSystem::ApplyAnimation(
        ComponentStorage<SkeletonComponent>& skeletons,
        Entity entity,
        AnimationComponent& anim,
        float time)
    {
        if (!skeletons.Has(entity))
            return;

        auto& sk = skeletons.Get(entity);

        if (sk.skeletonID == UUID::Null)
            return;

        Skeleton* skeleton =
            assetManager.GetSkeleton(sk.skeletonID);

        if (!skeleton)
            return;

        Animation* animation =
            assetManager.GetAnimation(anim.currentAnimationID);

        if (!animation)
            return;

        if (anim.finalBoneMatrices.size() != skeleton->bones.size())
        {
            anim.finalBoneMatrices.resize(
                skeleton->bones.size(),
                glm::mat4(1.0f));
        }

        ComputeBoneTransforms(
            *skeleton,
            *animation,
            time,
            anim.finalBoneMatrices);
    }

    void AnimationSystem::ComputeBoneTransforms(
        Skeleton& skeleton,
        Animation& animation,
        float time,
        std::vector<glm::mat4>& boneMatrices)
    {
        std::vector<glm::mat4> globalTransforms(skeleton.bones.size());

        for (size_t i = 0; i < skeleton.bones.size(); i++)
        {
            glm::mat4 localTransform(1.0f);

            int trackIndex = animation.boneTrackMap[i];

            if (trackIndex != -1)
            {
                auto& track = animation.tracks[trackIndex];

                glm::vec3 pos = InterpolatePosition(track, time, 1);
                glm::quat rot = InterpolateRotation(track, time, 1);
                //glm::vec3 scale = InterpolateScale(track, time, 1);

                localTransform = glm::translate(localTransform, pos);
                localTransform *= glm::toMat4(rot);
                localTransform = glm::scale(localTransform, glm::vec3(1.0f)); // for now no scaling
            }

            int parent = skeleton.bones[i].parentIndex;

            if (parent == -1)
                globalTransforms[i] = localTransform;
            else
                globalTransforms[i] = globalTransforms[parent] * localTransform;

            boneMatrices[i] =
                globalTransforms[i] * skeleton.bones[i].inverseBindMatrix;
        }
    }

    glm::vec3 AnimationSystem::InterpolatePosition(AnimationTrack& track, float time, int delta)
    {
        if (track.positions.size() == 1)
            return track.positions[0].position;


        for (size_t i = 0; i < track.positions.size() - 1; i+=delta)
        {
            if (time < track.positions[i + 1].timeStamp)
            {
                float t1 = track.positions[i].timeStamp;
                float t2 = track.positions[i + 1].timeStamp;

                float factor = (time - t1) / (t2 - t1);

                return glm::mix(
                    track.positions[i].position,
                    track.positions[i + 1].position,
                    factor
                );
            }
        }

        return track.positions.back().position;
    }

    glm::quat AnimationSystem::InterpolateRotation(AnimationTrack& track, float time, int delta)
    {
        if (track.rotations.size() == 1)
            return track.rotations[0].rotation;

        for (size_t i = 0; i < track.rotations.size() - 1; i+=delta)
        {
            if (time < track.rotations[i + 1].timeStamp)
            {
                float t1 = track.rotations[i].timeStamp;
                float t2 = track.rotations[i + 1].timeStamp;

                float factor = (time - t1) / (t2 - t1);

                return glm::slerp(
                    track.rotations[i].rotation,
                    track.rotations[i + 1].rotation,
                    factor
                );
            }
        }

        return track.rotations.back().rotation;
    }

    glm::vec3 AnimationSystem::InterpolateScale(AnimationTrack& track, float time, int delta)
    {
        if (track.scales.size() == 1)
            return track.scales[0].scale;

        for (size_t i = 0; i < track.scales.size() - 1; i+=delta)
        {
            if (time < track.scales[i + 1].timeStamp)
            {
                float t1 = track.scales[i].timeStamp;
                float t2 = track.scales[i + 1].timeStamp;

                float factor = (time - t1) / (t2 - t1);

                return glm::mix(
                    track.scales[i].scale,
                    track.scales[i + 1].scale,
                    factor
                );
            }
        }

        return track.scales.back().scale;
    }

}
#pragma once

namespace Lengine {
    struct AnimationComponent
    {
        std::vector<UUID> animationIDs;

        UUID currentAnimationID = UUID::Null;

        float currentTime = 0.0f;
        float playbackSpeed = 1.0f;

        bool looping = true;

        std::vector<glm::mat4> finalBoneMatrices;

        AnimationComponent()
            : currentAnimationID(UUID::Null),
            currentTime(0.0f),
            playbackSpeed(1.0f),
            looping(true)
        {
        }

        AnimationComponent(const std::vector<UUID>& animations)
            : animationIDs(animations),
            currentAnimationID(animations.empty() ? UUID::Null : animations[0]),
            currentTime(0.0f),
            playbackSpeed(1.0f),
            looping(true)
        {
        }
    };

  
}

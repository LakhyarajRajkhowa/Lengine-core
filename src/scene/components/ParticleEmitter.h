#pragma once

#include <glm/glm.hpp>
#include <random>

#include "utils/UUID.h"
#include "particles/ParticleEmitterAsset.h"

namespace Lengine {

    struct ParticleEmitter {
        UUID emitterAssetID = UUID::Null;

        glm::vec3 origin = glm::vec3(0.0f);
        glm::vec3 normal = glm::vec3(0, 1, 0);

        bool playing = true;

        float elapsedTime = 0.0f;
        float emitAccumulator = 0.0f; 

        std::mt19937 rng{ std::random_device{}() };

        void Restart(const ParticleEmitterAsset& asset) {
            elapsedTime = 0.0f;
            emitAccumulator = 0.0f;
            playing = true;
            rng = asset.useSeed ? std::mt19937(asset.seed)
                : std::mt19937(std::random_device{}());
        }
    };

}
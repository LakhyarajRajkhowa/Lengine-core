#pragma once
#include <vector>
#include <random>
#include <GL/glew.h>
#include <glm/glm.hpp>


#include "resources/AssetManager.h"
#include "scene/components/ComponentStorage.h"
#include "utils/UUID.h"

#include "Particle.h"
#include "ParticleEmitterAsset.h"

namespace Lengine {

    struct ParticleBatchKey {
        UUID textureID = UUID::Null;
        ParticleBlendMode blendMode;
        bool operator==(const ParticleBatchKey& o) const {
            return textureID == o.textureID && blendMode == o.blendMode;
        }
    };

    struct ParticleBatchKeyHash {
        size_t operator()(const ParticleBatchKey& k) const {
            return std::hash<UUID>{}(k.textureID) ^ (static_cast<size_t>(k.blendMode) << 1);
        }
    };

    class ParticleSystem {
    public:
        explicit ParticleSystem(AssetManager& assetManager, size_t maxParticles = 2096)
            : assetManager(assetManager), pool(maxParticles) {}

        void Init();
        void Update(float dt, ComponentStorage<ParticleEmitter>& emitters);
        void Render(const glm::mat4& view, const glm::mat4& projection);

        void SpawnBurst(
            const UUID& emitterAssetID,
            const glm::vec3& origin,
            const glm::vec3& normal
        );


        void Emit(
            std::mt19937& rng,
            const ParticleEmitterAsset& asset,
            const glm::vec3& origin,
            const glm::vec3& normal,
            int count
        );


        void UpdateEmitter(ParticleEmitter& emitter, float dt);

        size_t GetAliveCount() const { return aliveCount; }


        std::mt19937 MakeRng(const ParticleEmitterAsset& asset);

    private:
        struct InstanceData {
            glm::vec3 position;   
            glm::vec2 size;       
            glm::vec4 color;
            glm::vec4 brightness;
            float     rotation;
        };

        AssetManager& assetManager;

        std::vector<Particle> pool;
        size_t nextFree = 0;   
        size_t aliveCount = 0;

        std::vector<InstanceData> instanceScratch;


        struct PendingSubEmitter {
            UUID assetID;
            glm::vec3 position;
        };
        std::vector<PendingSubEmitter> pendingSubEmitters;

        GLuint quadVAO = 0;
        GLuint quadVBO = 0;
        GLuint instanceVBO = 0;

        GLSLProgram particleShader;


        static float RandRange(std::mt19937& rng, float lo, float hi);
        static int   RandRangeInt(std::mt19937& rng, int lo, int hi);
        static glm::vec3 RandomDirectionInCone(std::mt19937& rng, const glm::vec3& axis, float coneAngleDeg);


        static glm::vec3 SampleShapeOffset(
            std::mt19937& rng,
            ParticleShape shape,
            const glm::vec3& extents,
            float radius,
            const glm::vec3& normal
        );

        // Non-deterministic fallback generator
        std::mt19937 ambientRng{ std::random_device{}() };

        std::unordered_map<ParticleBatchKey, std::vector<InstanceData>, ParticleBatchKeyHash> batches;
    };

}
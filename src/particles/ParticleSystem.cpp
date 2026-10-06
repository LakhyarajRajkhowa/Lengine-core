#include "ParticleSystem.h"
#include "core/paths.h"
#include <random>
#include <cmath>

using namespace Lengine;

std::mt19937 ParticleSystem::MakeRng(const ParticleEmitterAsset& asset) {
    if (asset.useSeed) {
        return std::mt19937(asset.seed);
    }
    // Non-deterministic path: seed a fresh generator off the instance's
    // ambient entropy source rather than handing out ambientRng itself,
    // so callers get an independent, movable std::mt19937 either way.
    return std::mt19937(ambientRng());
}

float ParticleSystem::RandRange(std::mt19937& rng, float lo, float hi) {
    if (lo > hi) {
        return hi;
    }
    std::uniform_real_distribution<float> dist(lo, hi);
    return dist(rng);
}

int ParticleSystem::RandRangeInt(std::mt19937& rng, int lo, int hi) {
    if (lo > hi) {
        return hi;
    }
    std::uniform_int_distribution<int> dist(lo, hi);
    return dist(rng);
}


glm::vec3 ParticleSystem::RandomDirectionInCone(std::mt19937& rng, const glm::vec3& axis, float coneAngleDeg) {
    float coneRad = glm::radians(coneAngleDeg);

    float z = RandRange(rng, std::cos(coneRad), 1.0f);   
    float phi = RandRange(rng, 0.0f, glm::two_pi<float>());
    float r = std::sqrt(1.0f - z * z);

    glm::vec3 localDir(r * std::cos(phi), r * std::sin(phi), z);

    glm::vec3 up = glm::abs(axis.y) < 0.99f ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
    glm::vec3 tangent = glm::normalize(glm::cross(up, axis));
    glm::vec3 bitangent = glm::cross(axis, tangent);

    return glm::normalize(
        tangent * localDir.x + bitangent * localDir.y + axis * localDir.z
    );
}

glm::vec3 ParticleSystem::SampleShapeOffset(
    std::mt19937& rng,
    ParticleShape shape,
    const glm::vec3& extents,
    float radius,
    const glm::vec3& normal
) {
    switch (shape) {
    case ParticleShape::Box:
        return glm::vec3(
            RandRange(rng, -extents.x * 0.5f, extents.x * 0.5f),
            RandRange(rng, -extents.y * 0.5f, extents.y * 0.5f),
            RandRange(rng, -extents.z * 0.5f, extents.z * 0.5f)
        );

    case ParticleShape::Sphere:
    {

        glm::vec3 p;
        do {
            p = glm::vec3(RandRange(rng, -1.0f, 1.0f), RandRange(rng, -1.0f, 1.0f), RandRange(rng, -1.0f, 1.0f));
        } while (glm::dot(p, p) > 1.0f);
        return p * radius;
    }

    case ParticleShape::Circle:
    {

        float r = radius * std::sqrt(RandRange(rng, 0.0f, 1.0f));
        float theta = RandRange(rng, 0.0f, glm::two_pi<float>());

        glm::vec3 axis = glm::length(normal) > 0.0001f ? glm::normalize(normal) : glm::vec3(0, 1, 0);
        glm::vec3 up = glm::abs(axis.y) < 0.99f ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
        glm::vec3 tangent = glm::normalize(glm::cross(up, axis));
        glm::vec3 bitangent = glm::cross(axis, tangent);

        return tangent * (r * std::cos(theta)) + bitangent * (r * std::sin(theta));
    }

    case ParticleShape::Point:
    default:
        return glm::vec3(0.0f);
    }
}

void ParticleSystem::Init() {

    particleShader.compileShaders(
        Paths::Shaders + "particle.vert",
        Paths::Shaders + "particle.frag"
    );
    particleShader.linkShaders();

    float quadVertices[] = {
        // x,    y
        -0.5f, -0.5f,
         0.5f, -0.5f,
        -0.5f,  0.5f,
         0.5f,  0.5f,
    };

    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glGenBuffers(1, &instanceVBO);

    glBindVertexArray(quadVAO);

    // location 0: per-vertex quad corner
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

    // instance buffer — allocated empty, refilled with glBufferSubData each frame in Render()
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, pool.size() * sizeof(InstanceData), nullptr, GL_DYNAMIC_DRAW);

    // location 1: instance position (vec3)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
        (void*)offsetof(InstanceData, position));
    glVertexAttribDivisor(1, 1);

    // location 2: instance size (vec2 — width, height; was a single float packed with position)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
        (void*)offsetof(InstanceData, size));
    glVertexAttribDivisor(2, 1);

    // location 3: instance color (vec4)
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
        (void*)offsetof(InstanceData, color));
    glVertexAttribDivisor(3, 1);

    // location 4: instance brightness (vec4)
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
        (void*)offsetof(InstanceData, brightness));
    glVertexAttribDivisor(4, 1);

    // location 5: instance rotation (float)
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
        (void*)offsetof(InstanceData, rotation));
    glVertexAttribDivisor(5, 1);

    glBindVertexArray(0);

    instanceScratch.reserve(pool.size());

}

void ParticleSystem::SpawnBurst(
    const UUID& emitterAssetID,
    const glm::vec3& origin,
    const glm::vec3& normal
) {
    auto asset = assetManager.GetParticleEmitterAsset(emitterAssetID);
    if (!asset) return;


    std::mt19937 rng = MakeRng(*asset);

    int count = RandRangeInt(rng, asset->burstCountMin, asset->burstCountMax);
    Emit(rng, *asset, origin, normal, count);
}

void ParticleSystem::Emit(
    std::mt19937& rng,
    const ParticleEmitterAsset& asset,
    const glm::vec3& origin,
    const glm::vec3& normal,
    int count
) {
    if (count <= 0) return;

    glm::vec3 axis = glm::length(normal) > 0.0001f ? glm::normalize(normal) : glm::vec3(0, 1, 0);

    for (int i = 0; i < count; ++i) {
        Particle& p = pool[nextFree];
        nextFree = (nextFree + 1) % pool.size(); 

        glm::vec3 dir = axis;
        float     speed = RandRange(rng, asset.speedMin, asset.speedMax);

        glm::vec3 offset = SampleShapeOffset(rng, asset.shape, asset.shapeExtents, asset.shapeRadius, axis);

        p.position = origin + offset;
        p.velocity = dir * speed;
        p.colorStart = asset.colorStart;
        p.colorEnd = asset.colorEnd;
        p.color = asset.colorStart;
        p.brightness = asset.brightness;
        p.sizeStart = asset.sizeStart;
        p.sizeEnd = asset.sizeEnd;
        p.size = asset.sizeStart;

        p.rotationMode = asset.rotationMode;
        switch (asset.rotationMode) {
        case ParticleRotationMode::Fixed:
            p.rotation = glm::radians(asset.fixedRotationDeg);
            break;
        case ParticleRotationMode::AlignToVelocity:
            p.rotation = 0.0f;
            break;
        case ParticleRotationMode::Random:
        default:
            p.rotation = RandRange(rng, 0.0f, glm::two_pi<float>());
            break;
        }

        p.age = 0.0f;
        p.lifetime = RandRange(rng, asset.lifetimeMin, asset.lifetimeMax);
        p.gravity = asset.gravity;
        p.drag = asset.drag;
        p.alive = true;
        p.collideWithGround = asset.collideWithGround;
        p.groundHeight = asset.groundHeight;
        p.subEmitterAssetID = asset.subEmitterAssetID;
        p.subEmitterTrigger = asset.subEmitterTrigger;
        p.textureID = asset.textureID;
        p.blendMode = asset.blendMode;
    }
}

void ParticleSystem::UpdateEmitter(ParticleEmitter& emitter, float dt) {
    if (!emitter.playing) return;

    auto asset = assetManager.GetParticleEmitterAsset(emitter.emitterAssetID);
    if (!asset) return;

    emitter.elapsedTime += dt;

    if (emitter.elapsedTime < asset->startDelay) return;


    if (!asset->looping) {
        float activeTime = emitter.elapsedTime - asset->startDelay;
        if (activeTime >= asset->duration) {
            emitter.playing = false;
            return;
        }
    }

    // Accumulate fractional particles-per-second into whole particles.
    emitter.emitAccumulator += asset->emissionRate * dt;

    int toEmit = static_cast<int>(emitter.emitAccumulator);
    if (toEmit <= 0) return;

    emitter.emitAccumulator -= static_cast<float>(toEmit);

    Emit(emitter.rng, *asset, emitter.origin, emitter.normal, toEmit);
}

void ParticleSystem::Update(float dt, ComponentStorage<ParticleEmitter>& emitters) {
    auto& dense = emitters.GetDense();

    for (size_t i = 0; i < dense.size(); ++i) {
        ParticleEmitter& emitter = dense[i];
        UpdateEmitter(emitter, dt);
    }

    aliveCount = 0;
    pendingSubEmitters.clear();

    for (Particle& p : pool) {
        if (!p.alive) continue;

        p.age += dt;

        // Physics runs first so ground-collision checks below see this
        // frame's moved position, not last frame's.
        p.velocity.y += p.gravity * dt;
        p.velocity *= glm::clamp(1.0f - p.drag * dt, 0.0f, 1.0f);
        p.position += p.velocity * dt;

        bool expired = p.age >= p.lifetime;
        bool hitGround = p.collideWithGround && p.position.y <= p.groundHeight;

        if (expired || hitGround) {
            p.alive = false;

            bool spawnSub =
                p.subEmitterAssetID != UUID::Null &&
                (p.subEmitterTrigger == ParticleDeathSubEmitterTrigger::Both ||
                    (expired && p.subEmitterTrigger == ParticleDeathSubEmitterTrigger::OnExpire) ||
                    (hitGround && p.subEmitterTrigger == ParticleDeathSubEmitterTrigger::OnGroundHit));

            if (spawnSub) {
                glm::vec3 deathPos = p.position;
                if (hitGround) deathPos.y = p.groundHeight;
                pendingSubEmitters.push_back({ p.subEmitterAssetID, deathPos });
            }

            continue;
        }

        float t = p.NormalizedAge();
        p.size = glm::mix(p.sizeStart, p.sizeEnd, t);
        p.color = glm::mix(p.colorStart, p.colorEnd, t);


        ++aliveCount;
    }


    for (auto& pending : pendingSubEmitters) {
        SpawnBurst(pending.assetID, pending.position, glm::vec3(0, 1, 0));
    }
}
void ParticleSystem::Render(const glm::mat4& view, const glm::mat4& projection) {
    batches.clear();


    glm::vec3 camRight(view[0][0], view[1][0], view[2][0]);
    glm::vec3 camUp(view[0][1], view[1][1], view[2][1]);

    for (const Particle& p : pool) {
        if (!p.alive) continue;

        float renderRotation = p.rotation;

        if (p.rotationMode == ParticleRotationMode::AlignToVelocity) {
            float vx = glm::dot(p.velocity, camRight);
            float vy = glm::dot(p.velocity, camUp);


            if (vx * vx + vy * vy > 1e-8f) {
                renderRotation = std::atan2(vy, vx) - glm::half_pi<float>();
            }
        }

        batches[{ p.textureID, p.blendMode }].push_back(
            { p.position, p.size, p.color, p.brightness, renderRotation });
    }
    if (batches.empty()) return;

    particleShader.use();
    particleShader.setMat4("view", view);
    particleShader.setMat4("projection", projection);

    glBindVertexArray(quadVAO);
    glEnable(GL_BLEND);

    for (auto& [key, instances] : batches) {
        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0,
            instances.size() * sizeof(InstanceData), instances.data());


        switch (key.blendMode) {
        case ParticleBlendMode::Additive:
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            break;
        case ParticleBlendMode::NoBlend:
            glBlendFunc(GL_SRC_ALPHA, GL_ZERO);
            break;
        case ParticleBlendMode::AlphaBlend:
        default:
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            break;
        }

        bool hasTex = key.textureID != UUID::Null;
        if (hasTex) {
            if (auto tex = assetManager.getTexture(key.textureID)) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, tex->id); 
                particleShader.setInt("particleTex", 0);
            }
            else {
                hasTex = false;
            }
        }

        particleShader.setBool("useTexture", hasTex);

        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, (GLsizei)instances.size());
    }

    glDisable(GL_BLEND);
    glBindVertexArray(0);
    particleShader.unuse();
}
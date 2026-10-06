#pragma once
#include <glm/glm.hpp>

#include "utils/UUID.h"

namespace Lengine {


    enum class ParticleBlendMode {
        AlphaBlend,
        Additive,
        NoBlend
    };

    enum class ParticleShape {
        Point,   
        Box,     
        Sphere,  
        Circle,  
    };

    enum class ParticleRotationMode {
        Random,        
        Fixed,          
        AlignToVelocity,
    };

    enum class ParticleDeathSubEmitterTrigger {
        None,        
        OnExpire,    
        OnGroundHit,
        Both,       
    };

    struct Particle {
        glm::vec3 position = glm::vec3(0.0f);
        glm::vec3 velocity = glm::vec3(0.0f);

        glm::vec4 colorStart = glm::vec4(1.0f);
        glm::vec4 colorEnd = glm::vec4(1.0f);
        glm::vec4 color = glm::vec4(1.0f);
        glm::vec4 brightness = glm::vec4(1.0f);

        glm::vec2 sizeStart = glm::vec2(0.1f);
        glm::vec2 sizeEnd = glm::vec2(0.1f);
        glm::vec2 size = glm::vec2(0.1f);

        float rotation = 0.0f;
        ParticleRotationMode rotationMode = ParticleRotationMode::Random;

        float age = 0.0f;
        float lifetime = 1.0f;

        float gravity = -9.8f;
        float drag = 0.0f;

        bool alive = false;

        bool  collideWithGround = false;
        float groundHeight = 0.0f;

        UUID subEmitterAssetID = UUID::Null;
        ParticleDeathSubEmitterTrigger subEmitterTrigger = ParticleDeathSubEmitterTrigger::None;

        UUID textureID = UUID::Null;
        ParticleBlendMode blendMode = ParticleBlendMode::AlphaBlend;


        float NormalizedAge() const {
            return lifetime > 0.0f ? glm::clamp(age / lifetime, 0.0f, 1.0f) : 1.0f;
        }
    };

}
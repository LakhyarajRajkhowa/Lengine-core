#pragma once

#include <glm/glm.hpp>
#include <string>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <memory>

#include "Particle.h"


namespace Lengine {


    struct ParticleEmitterAsset {
        UUID        id = UUID::Null;
        std::string name;

        UUID              textureID = UUID::Null;
        ParticleBlendMode blendMode = ParticleBlendMode::AlphaBlend;

        int   burstCountMin = 12;
        int   burstCountMax = 20;

        float lifetimeMin = 0.4f;
        float lifetimeMax = 0.9f;

        float speedMin = 2.0f;
        float speedMax = 6.0f;

        float coneAngleDeg = 35.0f;

        ParticleRotationMode rotationMode = ParticleRotationMode::Random;
        float fixedRotationDeg = 0.0f;

        ParticleShape shape = ParticleShape::Point;
        glm::vec3     shapeExtents = glm::vec3(1.0f, 0.0f, 1.0f); 
        float         shapeRadius = 1.0f;                         

        float emissionRate = 10.0f;   // particles per second
        bool  looping = true;
        float duration = 5.0f;    // ignored if looping
        float startDelay = 0.0f;

        glm::vec2 sizeStart = glm::vec2(0.08f);
        glm::vec2 sizeEnd = glm::vec2(0.02f);

        glm::vec4 colorStart = { 0.5f, 0.0f, 0.0f, 1.0f };
        glm::vec4 colorEnd = { 0.2f, 0.0f, 0.0f, 0.0f };
        glm::vec4 brightness = glm::vec4(1.0f);

        float gravity = -9.8f;
        float drag = 1.5f;


        bool  collideWithGround = false;
        float groundHeight = 0.0f;

        UUID subEmitterAssetID = UUID::Null;
        ParticleDeathSubEmitterTrigger subEmitterTrigger = ParticleDeathSubEmitterTrigger::None;

        bool     useSeed = false;
        uint32_t seed = 0;
    };

    static glm::vec4 ParseVec4Csv(const std::string& csv)
    {
        glm::vec4 v(0.0f);
        std::stringstream ss(csv);
        std::string token;
        int i = 0;
        while (std::getline(ss, token, ',') && i < 4)
            v[i++] = std::stof(token);
        return v;
    }

    static std::shared_ptr<ParticleEmitterAsset> LoadParticleEmitter(
        const std::filesystem::path& filepath)
    {
        std::ifstream file(filepath);

        if (!file.is_open())
            return nullptr;

        auto asset = std::make_shared<ParticleEmitterAsset>();
        std::string line;

        std::getline(file, line);
        asset->name = line.substr(line.find('=') + 1);

        std::getline(file, line);
        asset->id = UUID(std::stoull(line.substr(line.find('=') + 1)));

        std::getline(file, line);
        asset->textureID = UUID(std::stoull(line.substr(line.find('=') + 1)));

        std::getline(file, line);
        asset->blendMode = static_cast<ParticleBlendMode>(
            std::stoi(line.substr(line.find('=') + 1)));

        std::getline(file, line); // blank line

        std::getline(file, line);
        asset->burstCountMin = std::stoi(line.substr(line.find('=') + 1));

        std::getline(file, line);
        asset->burstCountMax = std::stoi(line.substr(line.find('=') + 1));

        std::getline(file, line);
        asset->lifetimeMin = std::stof(line.substr(line.find('=') + 1));

        std::getline(file, line);
        asset->lifetimeMax = std::stof(line.substr(line.find('=') + 1));

        std::getline(file, line);
        asset->speedMin = std::stof(line.substr(line.find('=') + 1));

        std::getline(file, line);
        asset->speedMax = std::stof(line.substr(line.find('=') + 1));

        std::getline(file, line);
        asset->coneAngleDeg = std::stof(line.substr(line.find('=') + 1));

        std::getline(file, line); // blank line

        std::getline(file, line);
        {
            std::string v = line.substr(line.find('=') + 1);
            size_t comma = v.find(',');
            if (comma == std::string::npos)
                asset->sizeStart = glm::vec2(std::stof(v));
            else
                asset->sizeStart = glm::vec2(std::stof(v.substr(0, comma)), std::stof(v.substr(comma + 1)));
        }

        std::getline(file, line);
        {
            std::string v = line.substr(line.find('=') + 1);
            size_t comma = v.find(',');
            if (comma == std::string::npos)
                asset->sizeEnd = glm::vec2(std::stof(v));
            else
                asset->sizeEnd = glm::vec2(std::stof(v.substr(0, comma)), std::stof(v.substr(comma + 1)));
        }

        std::getline(file, line);
        asset->colorStart = ParseVec4Csv(line.substr(line.find('=') + 1));

        std::getline(file, line);
        asset->colorEnd = ParseVec4Csv(line.substr(line.find('=') + 1));

        std::getline(file, line);
        asset->brightness = ParseVec4Csv(line.substr(line.find('=') + 1));

        std::getline(file, line); // blank line

        std::getline(file, line);
        asset->gravity = std::stof(line.substr(line.find('=') + 1));

        std::getline(file, line);
        asset->drag = std::stof(line.substr(line.find('=') + 1));

        // -- shape block (optional — appended after the original format,
        //    so older asset files without it just stop here and keep defaults) --
        if (std::getline(file, line)) // blank line
        {
            if (std::getline(file, line))
                asset->shape = static_cast<ParticleShape>(std::stoi(line.substr(line.find('=') + 1)));

            if (std::getline(file, line))
                asset->shapeExtents = glm::vec3(ParseVec4Csv(line.substr(line.find('=') + 1)));

            if (std::getline(file, line))
                asset->shapeRadius = std::stof(line.substr(line.find('=') + 1));
        }

        // -- emission block (optional — appended after shape, same reasoning) --
        if (std::getline(file, line)) // blank line
        {
            if (std::getline(file, line))
                asset->emissionRate = std::stof(line.substr(line.find('=') + 1));

            if (std::getline(file, line))
                asset->looping = std::stoi(line.substr(line.find('=') + 1)) != 0;

            if (std::getline(file, line))
                asset->duration = std::stof(line.substr(line.find('=') + 1));

            if (std::getline(file, line))
                asset->startDelay = std::stof(line.substr(line.find('=') + 1));
        }

        // -- determinism block (optional, same reasoning as shape/emission) --
        if (std::getline(file, line)) // blank line
        {
            if (std::getline(file, line))
                asset->useSeed = std::stoi(line.substr(line.find('=') + 1)) != 0;

            if (std::getline(file, line))
                asset->seed = static_cast<uint32_t>(std::stoul(line.substr(line.find('=') + 1)));
        }

        // -- rotation block (optional, same reasoning as the others) --
        if (std::getline(file, line)) // blank line
        {
            if (std::getline(file, line))
                asset->rotationMode = static_cast<ParticleRotationMode>(
                    std::stoi(line.substr(line.find('=') + 1)));

            if (std::getline(file, line))
                asset->fixedRotationDeg = std::stof(line.substr(line.find('=') + 1));
        }

        // -- ground collision block (optional, same reasoning as the others) --
        if (std::getline(file, line)) // blank line
        {
            if (std::getline(file, line))
                asset->collideWithGround = std::stoi(line.substr(line.find('=') + 1)) != 0;

            if (std::getline(file, line))
                asset->groundHeight = std::stof(line.substr(line.find('=') + 1));
        }

        // -- death sub-emitter block (optional, same reasoning) --
        if (std::getline(file, line)) // blank line
        {
            if (std::getline(file, line))
                asset->subEmitterAssetID = UUID(std::stoull(line.substr(line.find('=') + 1)));

            if (std::getline(file, line))
                asset->subEmitterTrigger = static_cast<ParticleDeathSubEmitterTrigger>(
                    std::stoi(line.substr(line.find('=') + 1)));
        }

        return asset;
    }

}
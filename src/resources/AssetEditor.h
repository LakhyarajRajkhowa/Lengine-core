#pragma once
#include <filesystem>

#include "resources/assetDatabase/AssetDatabase.h"
namespace Lengine {
    class MaterialCreator {
    public:
        static UUID Create(const std::string& name)
        {
            UUID id;

            std::filesystem::path libPath = Paths::GameLibrary_Assets_Material + name + ".pbrmat";

            if (std::filesystem::exists(libPath))
            {
                std::cerr << "Material already exists\n";
                return UUID::Null;
            }

            // --- Create default material ---
            json j;
            j["albedo"] = { 1.0f, 1.0f, 1.0f };
            j["metallic"] = 0.0f;
            j["roughness"] = 0.5f;
            j["ao"] = 1.0f;
            j["normalStrength"] = 1.0f;
            j["textures"] = json::object();

            std::ofstream file(libPath);
            file << std::setw(4) << j;
            file.close();

            // --- Register asset ---
            AssetMetadata meta;
            meta.uuid = id;
            meta.name = name;
            meta.type = AssetType::Material;
            meta.libraryPath = libPath;
            meta.sourcePath = ""; // <-- important
            meta.thumbnailPath = Paths::Icons + "material_icon.png";

            AssetDatabase::RegisterAsset(meta);

            return id;
        }


    };

    class MaterialSaver {
    public:
        static void Save(
            const Material& mat,
            const std::filesystem::path& libPath
        )
        {
            json j;

            j["albedo"] = { mat.albedo.r, mat.albedo.g, mat.albedo.b };
            j["metallic"] = mat.metallic;
            j["roughness"] = mat.roughness;
            j["ao"] = mat.ao;
            j["normalStrength"] = mat.normalStrength;

            j["textures"]["albedo"] = mat.map_albedo_path;
            j["textures"]["normal"] = mat.map_normal_path;
            j["textures"]["metallic"] = mat.map_metallic_path;
            j["textures"]["roughness"] = mat.map_roughness_path;
            j["textures"]["ao"] = mat.map_ao_path;
            j["textures"]["metallicRoughness"] = mat.map_metallicRoughness_path;


            std::ofstream file(libPath);
            file << std::setw(4) << j;
        }


    };

    class BoneMaskSaver
    {
    public:
        static void Save(
            const BoneMask& mask,
            const std::filesystem::path& filepath)
        {
            std::ofstream file(filepath);

            if (!file.is_open())
                return;

            file << "Name=" << mask.name << "\n";
            file << "ID=" << (uint64_t)mask.id << "\n";
            file << "SkeletonID=" << (uint64_t)mask.skeletonId << "\n";

            file << "\n";

            file << "Bones=" << mask.boneNames.size() << "\n";

            for (const auto& [boneId, boneName] : mask.boneNames)
            {
                file << boneId << "," << boneName << "\n";
            }

            file << "\n";

            file << "Masks=" << mask.boneMask.size() << "\n";

            for (size_t i = 0; i < mask.boneMask.size(); i++)
            {
                file << i << "," << mask.boneMask[i] << "\n";
            }
        }
    };

    class ParticleEmitterSaver
    {
    public:
        static void Save(
            const ParticleEmitterAsset& asset,
            const std::filesystem::path& filepath)
        {
            std::ofstream file(filepath);

            if (!file.is_open())
                return;

            file << "Name=" << asset.name << "\n";
            file << "ID=" << (uint64_t)asset.id << "\n";
            file << "TextureID=" << (uint64_t)asset.textureID << "\n";
            file << "BlendMode=" << static_cast<int>(asset.blendMode) << "\n";

            file << "\n";

            file << "BurstCountMin=" << asset.burstCountMin << "\n";
            file << "BurstCountMax=" << asset.burstCountMax << "\n";
            file << "LifetimeMin=" << asset.lifetimeMin << "\n";
            file << "LifetimeMax=" << asset.lifetimeMax << "\n";
            file << "SpeedMin=" << asset.speedMin << "\n";
            file << "SpeedMax=" << asset.speedMax << "\n";
            file << "ConeAngleDeg=" << asset.coneAngleDeg << "\n";

            file << "\n";

            file << "SizeStart=" << asset.sizeStart.x << "," << asset.sizeStart.y << "\n";
            file << "SizeEnd=" << asset.sizeEnd.x << "," << asset.sizeEnd.y << "\n";

            file << "ColorStart=" << asset.colorStart.r << "," << asset.colorStart.g << ","
                << asset.colorStart.b << "," << asset.colorStart.a << "\n";
            file << "ColorEnd=" << asset.colorEnd.r << "," << asset.colorEnd.g << ","
                << asset.colorEnd.b << "," << asset.colorEnd.a << "\n";

            file << "Brightness=" << asset.brightness.r << "," << asset.brightness.g << ","
                << asset.brightness.b << "," << asset.brightness.a << "\n";

            file << "\n";

            file << "Gravity=" << asset.gravity << "\n";
            file << "Drag=" << asset.drag << "\n";

            // -- shape block (loader reads it optionally, trailing so files saved
            //    before shape existed still parse fine) --
            file << "\n";

            file << "Shape=" << static_cast<int>(asset.shape) << "\n";
            file << "ShapeExtents=" << asset.shapeExtents.x << "," << asset.shapeExtents.y << ","
                << asset.shapeExtents.z << "\n";
            file << "ShapeRadius=" << asset.shapeRadius << "\n";

            // -- emission block (same reasoning as shape above) --
            file << "\n";

            file << "EmissionRate=" << asset.emissionRate << "\n";
            file << "Looping=" << (asset.looping ? 1 : 0) << "\n";
            file << "Duration=" << asset.duration << "\n";
            file << "StartDelay=" << asset.startDelay << "\n";

            // -- determinism block (same reasoning — must come after emission,
            //    since the loader only tries to read it once emission succeeds) --
            file << "\n";

            file << "UseSeed=" << (asset.useSeed ? 1 : 0) << "\n";
            file << "Seed=" << asset.seed << "\n";

            // -- rotation block (must stay LAST — same reasoning as the others) --
            file << "\n";

            file << "RotationMode=" << static_cast<int>(asset.rotationMode) << "\n";
            file << "FixedRotationDeg=" << asset.fixedRotationDeg << "\n";

            // -- ground collision block (same reasoning as the others) --
            file << "\n";

            file << "CollideWithGround=" << (asset.collideWithGround ? 1 : 0) << "\n";
            file << "GroundHeight=" << asset.groundHeight << "\n";

            // -- death sub-emitter block (must stay LAST) --
            file << "\n";

            file << "SubEmitterAssetID=" << (uint64_t)asset.subEmitterAssetID << "\n";
            file << "SubEmitterTrigger=" << static_cast<int>(asset.subEmitterTrigger) << "\n";
        }
    };
    
    class BoneMaskCreator
    {
    public:
        static UUID Create(const std::string& name, UUID skeletonId = UUID::Null)
        {
            UUID id;

            std::string finalName = name;

            std::filesystem::path libPath =
                Paths::GameLibrary_Assets_BoneMask + finalName + ".bmask";

            int counter = 1;

            while (std::filesystem::exists(libPath))
            {
                finalName = name + "_" + std::to_string(counter++);

                libPath =
                    Paths::GameLibrary_Assets_BoneMask +
                    finalName +
                    ".bmask";
            }

            BoneMask mask;
            mask.name = finalName;
            mask.id = id;
            mask.skeletonId = skeletonId;

            BoneMaskSaver::Save(mask, libPath);

            AssetMetadata meta;
            meta.uuid = id;
            meta.name = finalName;
            meta.type = AssetType::BoneMask;
            meta.libraryPath = libPath;
            meta.sourcePath = "";
            meta.thumbnailPath = Paths::Icons + "bone_mask_icon.png";

            AssetDatabase::RegisterAsset(meta);

            return id;
        }
    };

    class ParticleEmitterCreator
    {
    public:
        static UUID Create(const std::string& name)
        {
            UUID id;

            std::string finalName = name;

            std::filesystem::path libPath =
                Paths::GameLibrary_Assets_Particle + finalName + ".particle";

            int counter = 1;

            while (std::filesystem::exists(libPath))
            {
                finalName = name + "_" + std::to_string(counter++);

                libPath =
                    Paths::GameLibrary_Assets_Particle +
                    finalName +
                    ".particle";
            }

            ParticleEmitterAsset asset;
            asset.id = id;
            asset.name = finalName;

            ParticleEmitterSaver::Save(asset, libPath);

            AssetMetadata meta;
            meta.uuid = id;
            meta.name = finalName;
            meta.type = AssetType::ParticleEmitter;
            meta.libraryPath = libPath;
            meta.sourcePath = "";
            meta.thumbnailPath = Paths::Icons + "particle_icon.png";

            AssetDatabase::RegisterAsset(meta);

            return id;
        }
    };

}

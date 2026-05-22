#pragma once

#include <filesystem>
#include <unordered_map>
#include "scene/Entity.h"

#include "MaterialRegistry.h"
#include "MeshRegistry.h"
#include "ShaderRegistry.h"
#include "TextureRegistry.h"

namespace Lengine {

    enum class AssetType {
        Unknown = -1,
        Mesh = 0,
        PhongMaterial = 1,
        Material = 2,
        Texture = 3,
        Shader = 4,
        Scene = 5,
        Prefab = 6,
        Skeleton = 7,
        Animation = 8,
        count = 9
    };

    enum class TextureMapType {
        Unknown = -1,
        Albedo = 0,
        Normal = 1,
        Metallic = 2,
        Roughness = 3,
        AmbientOcclusion = 4,
        MetallicRoughness = 5,
        count = 6
    };

    enum class TextureTargetType
    {
        GlobalMaterial,
        MeshRendererInstance
    };

    struct TextureLoadRequest
    {
        UUID textureID;
        UUID matID;
        Entity entityID;
        TextureMapType mapType;
        TextureTargetType targetType;

        bool srgb = false;
    };

    struct AssetRecord {
        UUID uuid;
        std::filesystem::path path;
        AssetType type;
    };

    inline std::string AssetTypeToString(AssetType type) {
        switch (type) {
        case AssetType::Mesh:            return "Mesh";
        case AssetType::PhongMaterial:   return "PhongMaterial";
        case AssetType::Material:        return "Material";
        case AssetType::Texture:         return "Texture";
        case AssetType::Shader:          return "Shader";
        case AssetType::Prefab:          return "Prefab";
        case AssetType::Skeleton:        return "Skeleton";
        case AssetType::Animation:       return "Animation";
        default: return "Unknown";
        }
    }

    inline AssetType StringToAssetType(const std::string& str)
    {
        if (str == "Mesh")           return AssetType::Mesh;
        if (str == "SubMesh")        return AssetType::Mesh;
        if (str == "PhongMaterial")  return AssetType::PhongMaterial;
        if (str == "Material")       return AssetType::Material;
        if (str == "Texture")        return AssetType::Texture;
        if (str == "Shader")         return AssetType::Shader;
        if (str == "Prefab")         return AssetType::Prefab;
        if (str == "Skeleton")       return AssetType::Skeleton;
        if (str == "Animation")      return AssetType::Animation;

        return AssetType::Unknown;
    }


    extern std::unordered_map<UUID, AssetRecord> assetRegistry;
}

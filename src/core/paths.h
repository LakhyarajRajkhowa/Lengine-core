#pragma once
#include <string>
#include <filesystem>

#include "core/settings.h"

namespace Lengine {

    struct Paths
    {
        static inline std::string EngineFolder;
        static inline std::string EditorFolder;
        static inline std::string GameExecutableFolder;

        static inline std::string Assets;
        static inline std::string Shaders;
        static inline std::string Materials;
        static inline std::string Textures;
        static inline std::string Mesh;
        static inline std::string Icons;
        static inline std::string Fonts;

        static inline std::string ActiveGameFolder;

        static inline std::string GameAssets;
        static inline std::string GameAssets_Shaders;
        static inline std::string GameAssets_Materials;
        static inline std::string GameAssets_Mesh;
        static inline std::string GameAssets_Textures;
        static inline std::string GameAssets_Prefab;
        static inline std::string GameScenes;

        static inline std::string GameAssetRegistryFolder;

        static inline std::string GameLibrary;
        static inline std::string GameLibrary_Assets;
        static inline std::string GameLibrary_Assets_Mesh;
        static inline std::string GameLibrary_Assets_Material;
        static inline std::string GameLibrary_Assets_Texture;
        static inline std::string GameLibrary_Assets_Prefab;
        static inline std::string GameLibrary_Assets_Skeleton;

        static inline std::string GameAssetDatabase;

        static inline std::string PBRVertexPath;
        static inline std::string PBRFragmentPath;


        static void setPaths(
            const std::string& gameFolderPath,
            const std::string& engineFolder,
            const std::string& editorFolder,
            const std::string& gameExectubalefolder
        )
        {
            ActiveGameFolder = gameFolderPath;
            EngineFolder = engineFolder;
            EditorFolder = editorFolder;
            GameExecutableFolder = gameExectubalefolder;

            Assets = engineFolder + "/assets/";
            Shaders = Assets + "Shaders/";
            Materials = Assets + "Materials/";
            Textures = Assets + "Textures/";
            Mesh = Assets + "Mesh/";
            Icons = Assets + "icons/";
            Fonts = Assets + "fonts/";


            GameAssets = ActiveGameFolder + "/Assets/";
            GameAssetRegistryFolder = ActiveGameFolder + "/AssetRegistry/";
            GameScenes = ActiveGameFolder + "/Scenes/";
            GameLibrary = ActiveGameFolder + "/Library/";

            GameAssets_Shaders = GameAssets + "Shaders/";
            GameAssets_Materials = GameAssets + "Materials/";
            GameAssets_Mesh = GameAssets + "Mesh/";
            GameAssets_Textures = GameAssets + "Textures/";
            GameAssets_Prefab = GameAssets + "Prefab/";

            GameLibrary_Assets = GameLibrary + "Assets/";
            GameLibrary_Assets_Mesh = GameLibrary_Assets + "Mesh/";
            GameLibrary_Assets_Material = GameLibrary_Assets + "Material/";
            GameLibrary_Assets_Texture = GameLibrary_Assets + "Texture/";
            GameLibrary_Assets_Prefab = GameLibrary_Assets + "Prefab/";
            GameLibrary_Assets_Skeleton = GameLibrary_Assets + "Skeleton/";

            GameAssetDatabase = ActiveGameFolder + "/AssetDatabase/";

            PBRVertexPath = Shaders + "pbr.vert";
            PBRFragmentPath = Shaders + "pbr.frag";

        }
    };



    struct ShaderPath
    {
        static std::string Vert(const std::string& name)
        {
            return Paths::Shaders + name + ".vert";
        }

        static std::string Frag(const std::string& name)
        {
            return Paths::Shaders + name + ".frag";
        }

        static std::string DefaultVert()
        {
            return Vert("defaultShader");
        }

        static std::string DefaultFrag()
        {
            return Frag("defaultShader");
        }

        static std::string LightVert()
        {
            return Vert("lightSource");
        }

        static std::string LightFrag()
        {
            return Frag("lightSource");
        }

        static std::string PBRVert()
        {
            return Vert("pbr");
        }

        static std::string PBRFrag()
        {
            return Frag("pbr");
        }

        static std::string DebugVert()
        {
            return Vert("debug");
        }

        static std::string DebugFrag()
        {
            return Frag("debug");
        }

        static std::string OutlineVert()
        {
            return Vert("outlineShader");
        }

        static std::string OutlineFrag()
        {
            return Frag("outlineShader");
        }

        static std::string GridVert()
        {
            return Vert("grid");
        }

        static std::string GridFrag()
        {
            return Frag("grid");
        }

        static std::string GeometryVert()
        {
            return Vert("geometry");
        }

        static std::string GeometryFrag()
        {
            return Frag("geometry");
        }

        static std::string DeferredPBRVert()
        {
            return Vert("deferredPBRLighting");
        }

        static std::string DeferredPBRFrag()
        {
            return Frag("deferredPBRLighting");
        }
    };

}
#pragma once
#include <vector>
#include "../core/paths.h"

namespace Lengine {

    struct ShaderAsset
    {
        std::string name;
        std::string vertexShaderPath;
        std::string fragmentShaderPath;
    };


    struct ShaderRegistry
    {
        // shader names (safe static constants)
        static constexpr const char* UNIVERSAL_PBR = "Universal PBR shader";
        static constexpr const char* DEBUG = "Debug shader";
        static constexpr const char* OUTLINE = "Outline shader";
        static constexpr const char* GIZMO_GRID = "Gizmo Grid";
        static constexpr const char* GEOMETRY = "Geometry";
        static constexpr const char* DEFERRED_PBR = "Deferred PBR shader";


        static std::vector<ShaderAsset> GetAllDefaults()
        {
            return {
                {
                    UNIVERSAL_PBR,
                    ShaderPath::PBRVert(),
                    ShaderPath::PBRFrag()
                },
                {
                    DEBUG,
                    ShaderPath::DebugVert(),
                    ShaderPath::DebugFrag()
                },
                {
                    OUTLINE,
                    ShaderPath::OutlineVert(),
                    ShaderPath::OutlineFrag()
                },
                {
                    GIZMO_GRID,
                    ShaderPath::GridVert(),
                    ShaderPath::GridFrag()
                },
                {
                    GEOMETRY,
                    ShaderPath::GeometryVert(),
                    ShaderPath::GeometryFrag()
                },
                 {
                    DEFERRED_PBR,
                    ShaderPath::DeferredPBRVert(),
                    ShaderPath::DeferredPBRFrag()
                }
            };
        }
    };

}
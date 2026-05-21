#pragma once

namespace Lengine
{
    enum class TextureUnit : unsigned int
    {
        // ----- Material Maps -----
        gPosition = 0,
        Albedo = 1,
        Normal = 2,
        Metallic = 3,
        Roughness = 4,
        AO = 5,
        MetallicRoughness = 6,
        Height = 7,

        // ----- Environment / IBL -----
        Irradiance = 8,
        Prefilter = 9,
        BRDF_LUT = 10,

        // ----- Shadow Maps -----
        Shadow2D = 11,   // Directional / Spot
        ShadowCube = 12,   // Point light

        // ----- Post Processing -----
        SceneColor = 13,
        SceneDepth = 14,
        Bloom = 15
    };
}

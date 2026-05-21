#version 330 core

layout(location = 0) out vec4 gPosition;
layout(location = 1) out vec4 gNormal;
layout(location = 2) out vec4 gAlbedo;
layout(location = 3) out vec4 gMaterial;

in vec2 TexCoords;
in vec3 WorldPos;
in mat3 TBN;

struct PBRMaterial
{
    vec3 albedo;
    float metallic;
    float roughness;
    float ao;

    sampler2D albedoMap;
    bool hasAlbedoMap;

    sampler2D normalMap;
    bool hasNormalMap;
    float normalStrength;

    sampler2D metallicRoughnessMap;
    bool hasMetallicRoughnessMap;

    sampler2D metallicMap;
    bool hasMetallicMap;

    sampler2D roughnessMap;
    bool hasRoughnessMap;
};

uniform PBRMaterial material;

vec3 GetNormal()
{
    if (material.hasNormalMap)
    {
        vec3 n = texture(material.normalMap, TexCoords).rgb;
        n = n * 2.0 - 1.0;
        n.xy *= material.normalStrength;
        return normalize(TBN * n);
    }
    return normalize(TBN[2]);
}

void main()
{
    vec4 albedoSample =
        vec4(material.albedo, 1.0);

    if(material.hasAlbedoMap)
    {
        albedoSample *=
            texture(material.albedoMap, TexCoords);
    }

    vec3 albedo = albedoSample.rgb;
    float alpha = albedoSample.a;

    float metallic =
        material.metallic;

    float roughness =
        material.roughness;

    if(material.hasMetallicRoughnessMap)
    {
        vec3 orm =
            texture(
                material.metallicRoughnessMap,
                TexCoords
            ).rgb;

        roughness *= orm.g;
        metallic *= orm.b;
    }
    else
    {
        if(material.hasMetallicMap)
        {
            metallic *=
                texture(
                    material.metallicMap,
                    TexCoords
                ).r;
        }

        if(material.hasRoughnessMap)
        {
            roughness *=
                texture(
                    material.roughnessMap,
                    TexCoords
                ).r;
        }
    }

    roughness =
        clamp(roughness, 0.04, 1.0);

    gPosition = vec4(WorldPos, 1.0);

    gNormal = vec4(GetNormal(), 1.0);

    gAlbedo = vec4(albedo, alpha);

    gMaterial = vec4(roughness, metallic, 0.0, 1.0);

}
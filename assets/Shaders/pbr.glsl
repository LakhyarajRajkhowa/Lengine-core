#ifndef PBR_GLSL
#define PBR_GLSL

const float PI = 3.14159265359;

// ---------------- Material ----------------

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

    sampler2D aoMap;
    bool hasAOMap;

    sampler2D metallicMap;
    bool hasMetallicMap;

    sampler2D roughnessMap;
    bool hasRoughnessMap;
};

// ---------------- PBR functions ----------------

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a  = roughness * roughness;
    float a2 = a * a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    return a2 / (PI * denom * denom);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    return GeometrySchlickGGX(max(dot(N, V), 0.0), roughness) *
           GeometrySchlickGGX(max(dot(N, L), 0.0), roughness);
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0)
           * pow(1.0 - cosTheta, 5.0);
}

vec3 EvaluatePBR(
    vec3 N, vec3 V, vec3 L,
    vec3 radiance,
    vec3 albedo,
    float metallic,
    float roughness,
    vec3 F0
){
    vec3 H = normalize(V + L);

    float NDF = DistributionGGX(N, H, roughness);
    float G   = GeometrySmith(N, V, L, roughness);
    vec3  F   = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 specular = (NDF * G * F) /
        max(4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0), 0.0001);

    vec3 kS = F;
    vec3 kD = (1.0 - kS) * (1.0 - metallic);

    float NdotL = max(dot(N, L), 0.0);
    return (kD * albedo / PI + specular) * radiance * NdotL;
}



#endif
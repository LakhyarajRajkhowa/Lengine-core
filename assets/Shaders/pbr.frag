#version 330 core
#define MAX_LIGHTS 32

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

in vec2 TexCoords;
in vec3 WorldPos;
in mat3 TBN;

in vec4 FragPosLightSpace;


uniform vec3 cameraPos;

// ---------------- Light arrays ----------------
uniform vec3  lightPositions[MAX_LIGHTS];   // point / spot
uniform vec3  lightDirections[MAX_LIGHTS];  // dir / spot (normalized)
uniform vec3  lightColors[MAX_LIGHTS];
uniform int   lightTypes[MAX_LIGHTS];        // 0=Dir, 1=Point, 2=Spot
uniform float lightIntensities[MAX_LIGHTS];
uniform bool  lightCastShadow[MAX_LIGHTS];

uniform float lightRanges[MAX_LIGHTS];
uniform float lightInnerAngles[MAX_LIGHTS]; // cos(inner)
uniform float lightOuterAngles[MAX_LIGHTS]; // cos(outer)
uniform int   lightCount;

const float PI = 3.14159265359;

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

uniform PBRMaterial material;

// IBL
uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D   brdfLUT; 

uniform float envIntensity;
uniform vec3 envTint;
uniform mat3 envRotation;

// Shadow map
uniform sampler2D shadowMap;
uniform samplerCube shadowCubeMap;
uniform float farPlane;


float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if(projCoords.x < 0.0 || projCoords.x > 1.0 ||
       projCoords.y < 0.0 || projCoords.y > 1.0 ||
       projCoords.z > 1.0)
        return 0.0;

    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;

    float bias = max(0.01 * (1.0 - max(dot(normal, lightDir),0.0)), 0.001);

    float shadow = 0.0;

    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);

    for(int x = -1; x <= 1; ++x)
    for(int y = -1; y <= 1; ++y)
    {
        float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x,y)*texelSize).r;
        shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
    }

    shadow /= 9.0;

    return shadow;
}
vec3 sampleOffsetDirections[20] = vec3[](
   vec3( 1,  1,  1), vec3(-1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1),
   vec3( 1,  1, -1), vec3(-1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1),
   vec3( 1,  1,  0), vec3(-1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0),
   vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
   vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0,  1, -1), vec3( 0, -1, -1)
);
float ShadowCubeMapCalculation(vec3 fragPos, vec3 lightPos)
{
    vec3 fragToLight = fragPos - lightPos;
    float currentDepth = length(fragToLight);

    float shadow = 0.0;
    float bias = max(0.05 * (currentDepth / farPlane), 0.005);

    int samples  = 20;
    float viewDistance = length(cameraPos - fragPos);
    float diskRadius = 0.05;
    for(int i = 0; i < samples; ++i)
    {
        float closestDepth = texture(shadowCubeMap, fragToLight + sampleOffsetDirections[i] * diskRadius).r;
        closestDepth *= farPlane;   // undo mapping [0;1]
        if(currentDepth - bias > closestDepth)
            shadow += 1.0;
    }
    shadow /= float(samples);  

    return shadow;
}
// ---------------- Helpers ----------------

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

vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0)
           * pow(1.0 - cosTheta, 5.0);
}
// ---------------- Main ----------------

void main()
{
    vec3 N = GetNormal();
    vec3 V = normalize(cameraPos - WorldPos);

    float metallic  = material.metallic;
    float roughness = material.roughness;
    float ao        = material.ao;

    vec4 albedoSample = vec4(material.albedo, 1.0);
    if (material.hasAlbedoMap)
        albedoSample *= texture(material.albedoMap, TexCoords);

    vec3 albedo = albedoSample.rgb;
    float alpha = albedoSample.a;

    if (material.hasMetallicRoughnessMap)
    {
        vec3 orm = texture(material.metallicRoughnessMap, TexCoords).rgb;
        ao        *= orm.r;
        roughness *= orm.g;
        metallic  *= orm.b;
    }
    else
    {
        if (material.hasMetallicMap)
            metallic *= texture(material.metallicMap, TexCoords).r;
        if (material.hasRoughnessMap)
            roughness *= texture(material.roughnessMap, TexCoords).r;
        if (material.hasAOMap)
            ao *= texture(material.aoMap, TexCoords).r;
    }

    roughness = clamp(roughness, 0.04, 1.0);
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    vec3 Lo = vec3(0.0);


    // ---------------- Lighting loop ----------------
    for (int i = 0; i < lightCount; i++)
    {
        vec3 L;
        vec3 radiance = lightColors[i] * lightIntensities[i];
        float attenuation = 1.0;

        float shadow = 0.0;
        if (lightTypes[i] == 0) // Directional
        {
            L = normalize(-lightDirections[i]);

            if(lightCastShadow[i]){
                shadow = ShadowCalculation(FragPosLightSpace, N, L);
            }
        }
        else
        {
            if(lightCastShadow[i]){
                shadow = ShadowCubeMapCalculation(WorldPos, lightPositions[i]);
            }

            vec3 toLight = lightPositions[i] - WorldPos;
            float dist = length(toLight);
            L = toLight / max(dist, 0.0001);

            // distance attenuation
            float range = lightRanges[i];
            float falloff = clamp(1.0 - dist / range, 0.0, 1.0);
            attenuation = falloff * falloff;

            // spotlight cone
            if (lightTypes[i] == 2)
            {
                float theta = dot(L, normalize(-lightDirections[i]));
                float inner = lightInnerAngles[i];
                float outer = lightOuterAngles[i];
                float epsilon = inner - outer;
                float cone = clamp((theta - outer) / epsilon, 0.0, 1.0);
                attenuation *= cone;
            }

            
        }

        radiance *= attenuation;

        vec3 direct = EvaluatePBR(
            N, V, L,
            radiance,
            albedo,
            metallic,
            roughness,
            F0
        );

        direct *= (1.0 - shadow);

        Lo += direct;
    }

    vec3 R = reflect(-V, N);
    // ---------------- IBL Ambient ----------------
    vec3 F = FresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);

    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;	  
    
    vec3 irradiance = texture(irradianceMap, envRotation * N).rgb;
    irradiance *= envTint;
    vec3 diffuse    = irradiance * albedo;
    
    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilteredColor = textureLod(prefilterMap, R,  roughness * MAX_REFLECTION_LOD).rgb;   
    prefilteredColor *= envTint;

    vec2 envBRDF  = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 specular = prefilteredColor * (F * envBRDF.x + envBRDF.y);
    
    vec3 ambient = (kD * diffuse + specular) * ao * envIntensity; 

    vec3 finalColor = ambient + Lo;

    FragColor = vec4(finalColor, alpha);

    float brightness = dot(finalColor, vec3(0.2126, 0.7152, 0.0722));
    BrightColor = brightness > 1.0
        ? vec4(finalColor, 1.0)
        : vec4(0.0, 0.0, 0.0, 1.0);
}

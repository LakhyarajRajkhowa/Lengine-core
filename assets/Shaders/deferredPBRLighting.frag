#version 330 core

layout (location = 0) out vec4 FragColor;

#define PI  3.14159265359


in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedo;
uniform sampler2D gMaterial;


#define MAX_LIGHTS 32

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

uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D   brdfLUT; 

uniform float envIntensity;
uniform vec3 envTint;
uniform mat3 envRotation;

uniform sampler2D shadowMap;
uniform samplerCube shadowCubeMap;
uniform mat4 lightSpaceMatrix;
uniform float farPlane;

uniform vec3 cameraPos;

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

void main()
{
    // ---------------- GBUFFER READ ----------------

    vec3 WorldPos =
        texture(gPosition, TexCoords).rgb;

    vec3 N =
        normalize(texture(gNormal, TexCoords).rgb);

    vec3 albedo =
        texture(gAlbedo, TexCoords).rgb;

    vec2 materialData =
        texture(gMaterial, TexCoords).rg;

    float roughness = materialData.r;
    float metallic  = materialData.g;

    vec3 V = normalize(cameraPos - WorldPos);

    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    vec3 Lo = vec3(0.0);


    // LIGHTING
    for(int i = 0; i < lightCount; i++)
    {
        vec3 L;
        vec3 radiance =
            lightColors[i] *
            lightIntensities[i];

        float attenuation = 1.0;

        float shadow = 0.0;

        // Directional
        if(lightTypes[i] == 0)
        {
            L = normalize(-lightDirections[i]);
            vec4 fragPosLightSpace =
            lightSpaceMatrix * vec4(WorldPos, 1.0);

            shadow =
                ShadowCalculation(
                    fragPosLightSpace,
                    N,
                    L
                );

        }
        else
        {
            vec3 toLight =
                lightPositions[i] - WorldPos;

            float dist = length(toLight);

            L = toLight / max(dist, 0.0001);

            float range = lightRanges[i];

            float falloff =
                clamp(1.0 - dist / range, 0.0, 1.0);

            attenuation = falloff * falloff;

            // Spot
            if(lightTypes[i] == 2)
            {
                float theta =
                    dot(L, normalize(-lightDirections[i]));

                float inner = lightInnerAngles[i];
                float outer = lightOuterAngles[i];

                float epsilon = inner - outer;

                float cone =
                    clamp((theta - outer) / epsilon, 0.0, 1.0);

                attenuation *= cone;
            }

            if(lightCastShadow[i]){
                shadow = ShadowCubeMapCalculation(WorldPos, lightPositions[i]);
            }
        }

        radiance *= attenuation;

        vec3 direct = EvaluatePBR(
            N,
            V,
            L,
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

    vec3 F =
        FresnelSchlickRoughness(
            max(dot(N, V), 0.0),
            F0,
            roughness
        );

    vec3 kS = F;
    vec3 kD = (1.0 - kS) * (1.0 - metallic);

    vec3 irradiance =
        texture(irradianceMap,
                envRotation * N).rgb;

    irradiance *= envTint;

    vec3 diffuseIBL =
        irradiance * albedo;

    const float MAX_REFLECTION_LOD = 4.0;

    vec3 prefilteredColor =
        textureLod(
            prefilterMap,
            envRotation * R,
            roughness * MAX_REFLECTION_LOD
        ).rgb;

    prefilteredColor *= envTint;

    vec2 envBRDF =
        texture(
            brdfLUT,
            vec2(max(dot(N, V), 0.0), roughness)
        ).rg;

    vec3 specularIBL =
        prefilteredColor *
        (F * envBRDF.x + envBRDF.y);

    vec3 ambient =
        (kD * diffuseIBL + specularIBL)
        * envIntensity;

    vec3 finalColor = Lo + ambient;

    FragColor = vec4(finalColor, 1.0);
}
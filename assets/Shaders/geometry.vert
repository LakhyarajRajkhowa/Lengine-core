#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;
layout (location = 5) in ivec4 boneIds;
layout (location = 6) in vec4 weights;

out vec2 TexCoords;
out vec3 WorldPos;
out mat3 TBN;

uniform mat4 view;
uniform mat4 projection;
uniform mat4 model;

const int MAX_BONES = 200;
const int MAX_BONE_INFLUENCE = 4;

uniform mat4 finalBonesMatrices[MAX_BONES];
uniform bool useSkeleton;

void main()
{
    mat4 skinMatrix = mat4(1.0);

    if(useSkeleton)
    {
        bool validBone = false;

        for(int i = 0; i < MAX_BONE_INFLUENCE; i++)
        {
            if(boneIds[i] >= 0 &&
               boneIds[i] < MAX_BONES &&
               weights[i] > 0.0)
            {
                validBone = true;
            }
        }

        if(validBone)
        {
            skinMatrix = mat4(0.0);

            for(int i = 0; i < MAX_BONE_INFLUENCE; i++)
            {
                if(boneIds[i] >= 0 &&
                   boneIds[i] < MAX_BONES)
                {
                    skinMatrix +=
                        finalBonesMatrices[boneIds[i]]
                        * weights[i];
                }
            }
        }
    }

    vec4 skinnedPosition =
        skinMatrix * vec4(aPos, 1.0);

    mat3 normalMatrix =
        mat3(skinMatrix);

    vec3 skinnedNormal =
        normalMatrix * aNormal;

    vec3 skinnedTangent =
        normalMatrix * aTangent;

    vec3 skinnedBitangent =
        normalMatrix * aBitangent;

    vec4 worldPos =
        model * skinnedPosition;

    WorldPos = worldPos.xyz;

    TexCoords = aTexCoords;

    vec3 T =
        normalize(mat3(model) * skinnedTangent);

    vec3 B =
        normalize(mat3(model) * skinnedBitangent);

    vec3 N =
        normalize(mat3(model) * skinnedNormal);

    TBN = mat3(T, B, N);

    gl_Position =
        projection * view * worldPos;
}
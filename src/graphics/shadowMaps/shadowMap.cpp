#include "shadowMap.h"

using namespace Lengine;

void ShadowMap::Init() {

    glGenFramebuffers(1, &shadowFBO);

    // Depth texture
    glGenTextures(1, &shadowDepthTex);
    glBindTexture(GL_TEXTURE_2D, shadowDepthTex);
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
        SHADOW_RES, SHADOW_RES, 0,
        GL_DEPTH_COMPONENT, GL_FLOAT, nullptr
    );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Prevent shadow edge artifacts
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // Attach to FBO
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_DEPTH_ATTACHMENT,
        GL_TEXTURE_2D,
        shadowDepthTex,
        0
    );

    // No color buffer
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    depthShader.compileShaders(Paths::Shaders + "depthShader.vert", Paths::Shaders + "depthShader.frag");
    depthShader.linkShaders();
}


void ShadowMap::renderDepthMap(
    const std::vector<Entity> entities,
    const ComponentStorage<TransformComponent>& trs,
    const ComponentStorage<MeshFilter>& mfs,
    const ComponentStorage<AnimationComponent>& anims,
    const Entity& mainDirectionalLight,
    AssetManager& assetManager,
    const glm::vec3& camPos
) {
    if (mainDirectionalLight == NullEntity || !trs.Has(mainDirectionalLight) || prevLight != mainDirectionalLight) {

        prevLight = mainDirectionalLight;
        // clear previous frame shadow
        glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        return;
    }

    shadowFar = shadowExtent * 4.0f;

    auto& lightTf = trs.Get(mainDirectionalLight);

    glm::vec3 lightDir = glm::normalize(lightTf.localRotation * glm::vec3(0.0f, -1.0f, 0.0f));

    // Stable up vector — avoids degenerate lookAt when light is near-vertical
    glm::vec3 up = (glm::abs(lightDir.y) > 0.99f)
        ? glm::vec3(0.0f, 0.0f, 1.0f)
        : glm::vec3(0.0f, 1.0f, 0.0f);

    glm::vec3 shadowCenter = camPos;
    glm::vec3 lightPos = shadowCenter - lightDir * (shadowFar * 0.5f);

    glm::mat4 lightView = glm::lookAt(lightPos, lightPos + lightDir, up);
    glm::mat4 lightProj = glm::orthoRH_ZO(-shadowExtent, shadowExtent,
        -shadowExtent, shadowExtent,
        shadowNear, shadowFar);

    // ─── Texel snapping: quantize translation to shadow-map texel grid ───
    {
        glm::mat4 shadowMVP = lightProj * lightView;
        glm::vec4 shadowOrigin = shadowMVP * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        shadowOrigin *= (float)SHADOW_RES * 0.5f;

        glm::vec4 roundedOrigin = glm::round(shadowOrigin);
        glm::vec4 roundOffset = roundedOrigin - shadowOrigin;
        roundOffset *= 2.0f / (float)SHADOW_RES;
        roundOffset.z = 0.0f;
        roundOffset.w = 0.0f;

        lightProj[3] += roundOffset;
    }

    lightSpaceMat = lightProj * lightView;

    depthShader.use();
    depthShader.setMat4("lightSpaceMatrix", lightSpaceMat);

    glViewport(0, 0, SHADOW_RES, SHADOW_RES);

    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
    glClear(GL_DEPTH_BUFFER_BIT);


    for (auto& e : entities)
    {
        if (!trs.Has(e) || !mfs.Has(e) || e == mainDirectionalLight) continue;

        auto& tr = trs.Get(e);
        auto& mf = mfs.Get(e);
        auto& meshID = mf.meshID;
        auto* mesh = assetManager.GetSubmesh(meshID);

        if (!mesh) continue;

        depthShader.setMat4("model", tr.worldMatrix);


        bool hasSkeleton = false;
        const AnimationComponent* anim = nullptr;
        std::vector<glm::mat4> allBones;
        std::vector<glm::mat4> bones;
        std::vector<int> pallete;

        if (anims.Has(mf.rootParent))
        {
            anim =
                &anims.Get(mf.rootParent);

            if (!anim->finalBoneMatrices.empty()
                && !mesh->bonePalette.empty()) 
            {
                hasSkeleton = true;
                allBones = anim->finalBoneMatrices;
                pallete = mesh->bonePalette;
            }
        }

        if (hasSkeleton)
        {
            depthShader.setBool("useSkeleton", hasSkeleton);

            bones.resize(pallete.size());
            for (size_t i = 0; i < pallete.size(); ++i)
                bones[i] = allBones[pallete[i]];

            for (int i = 0; i < (int)bones.size(); ++i)
                depthShader.setMat4("finalBonesMatrices[" + std::to_string(i) + "]", bones[i]);
        }


       mesh->draw();
    }

    depthShader.unuse();


    glBindFramebuffer(GL_FRAMEBUFFER, 0);


}
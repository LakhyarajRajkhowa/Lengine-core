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

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

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
    const std::vector<std::unique_ptr<Entity>>& entities,
    const ComponentStorage<TransformComponent>& trs,
    const ComponentStorage<MeshFilter>& mfs,
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

    glm::mat4 lightSpaceProj =
        glm::ortho(
            -20.0f, 20.0f,
            -20.0f, 20.0f,
            nearPlane, farPlane
        );

    auto& lightTf = trs.Get(mainDirectionalLight);
    glm::vec3 lightDir = glm::normalize(lightTf.localRotation * glm::vec3(0.0f, -1.0f, 0.0f));

    glm::vec3 center = camPos;  // anchor to camera

    glm::vec3 lightPos = center - lightDir * 20.0f; // move back along light dir

    glm::mat4 lightView = glm::lookAt(
        lightPos,
        center,
        glm::vec3(0, 1, 0)
    );



    glm::mat4 lightSpaceMat = lightSpaceProj * lightView;

    depthShader.use();
    depthShader.setMat4("lightSpaceMatrix", lightSpaceMat);

    glViewport(0, 0, SHADOW_RES, SHADOW_RES);

    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
    glClear(GL_DEPTH_BUFFER_BIT);


    for (auto& e : entities)
    {
        if (!trs.Has(*e) || !mfs.Has(*e) || *e == mainDirectionalLight) continue;;

        auto& tr = trs.Get(*e);
        depthShader.setMat4("model", tr.worldMatrix);
        auto& mf = mfs.Get(*e);

        auto* mesh = assetManager.GetSubmesh(mf.meshID);
        if (mesh) {
            mesh->draw();
        }
    }

    depthShader.unuse();


    glBindFramebuffer(GL_FRAMEBUFFER, 0);


}
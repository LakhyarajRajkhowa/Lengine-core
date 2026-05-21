#include "DeferredRenderer.h"

using namespace Lengine;

void DeferredRenderer::bindCameraUniforms(
    GLSLProgram& shader,
    const glm::mat4& model,
    Camera3d& editorCamera
) {

    shader.setMat4("model", model);
    shader.setMat4("view", editorCamera.getViewMatrix());
    shader.setMat4("projection", editorCamera.getProjectionMatrix());
    shader.setVec3("cameraPos", editorCamera.getCameraPosition());
    shader.setVec3("viewPos", editorCamera.getCameraPosition());

}


void DeferredRenderer::bindPBRMaterial(
    GLSLProgram& shader,
    const ResolvedMaterial& mat
) {
    shader.setVec3("material.albedo", mat.albedo);
    shader.setFloat("material.metallic", mat.metallic);
    shader.setFloat("material.roughness", mat.roughness);
    shader.setFloat("material.ao", mat.ao);
    shader.setFloat("material.normalStrength", mat.normalStrength);
}


void DeferredRenderer::bindTexture(
    GLSLProgram& shader,
    AssetManager& assetManager,
    const UUID& texID,
    const bool UseTexture,
    const char* hasUniform,
    const char* samplerUniform,
    GLenum textureUnit
) {
    bool hasTexture = (texID != UUID::Null && UseTexture);
    shader.setBool(hasUniform, hasTexture);

    if (!UseTexture) return;

    glActiveTexture(textureUnit);

    if (!hasTexture) {
        glBindTexture(GL_TEXTURE_2D, 0);
        return;
    }

    GLTexture* tex = assetManager.getTexture(texID);
    if (!tex) {
        glBindTexture(GL_TEXTURE_2D, 0);
        return;
    }

    glBindTexture(GL_TEXTURE_2D, tex->id);

    shader.setInt(samplerUniform, textureUnit - GL_TEXTURE0);
}



void DeferredRenderer::drawSubMesh(
    Mesh& sm,
    GLSLProgram& shader
) {
    sm.draw();
}

void DeferredRenderer::bindShadowMapUniforms(
    GLSLProgram& shader,
    ShadowMap& shadowMap,
    const TransformComponent& lightTransform,
    const glm::vec3& camPos
) {

    glm::mat4 lightSpaceProj =
        glm::ortho(
            -20.0f, 20.0f,
            -20.0f, 20.0f,
            shadowMap.nearPlane, shadowMap.farPlane
        );


    glm::vec3 lightDir = glm::normalize(lightTransform.localRotation * glm::vec3(0.0f, -1.0f, 0.0f));

    glm::vec3 center = camPos;  // anchor to camera

    glm::vec3 lightPos = center - lightDir * 20.0f; // move back along light dir

    glm::mat4 lightView = glm::lookAt(
        lightPos,
        center,
        glm::vec3(0, 1, 0)
    );



    glm::mat4 lightSpaceMat = lightSpaceProj * lightView;
    shader.setMat4(
        "lightSpaceMatrix",
        lightSpaceMat
    );

    glActiveTexture(GL_TEXTURE0 + static_cast<unsigned int>(TextureUnit::Shadow2D));
    glBindTexture(GL_TEXTURE_2D, shadowMap.getDepthTexture());
    shader.setInt("shadowMap", static_cast<unsigned int>(TextureUnit::Shadow2D));
}

void DeferredRenderer::bindPointShadowUniforms(
    GLSLProgram& shader,
    ShadowCubeMap& shadowCubeMap
) {
    glActiveTexture(GL_TEXTURE0 + static_cast<unsigned int>(TextureUnit::ShadowCube));
    glBindTexture(GL_TEXTURE_CUBE_MAP, shadowCubeMap.getDepthCubeMap());
    shader.setInt("shadowCubeMap", static_cast<unsigned int>(TextureUnit::ShadowCube));

    shader.setFloat("farPlane", shadowCubeMap.getFarPlane());
}

void DeferredRenderer::RenderGeometry(
    const RenderContext& ctx
) {
    const Scene* activeScene = ctx.scene;

    GLSLProgram* geomShader = assetManager.getShader(ShaderRegistry::GEOMETRY);
    geomShader->use();



    const auto& entities = activeScene->getEntities();
    auto& meshRenderers = activeScene->MeshRenderers();
    auto& meshFilters = activeScene->MeshFilters();
    auto& transforms = activeScene->Transforms();
    auto& animations = activeScene->Animations();


    // Bind camera once (view & projection are global)
    geomShader->setMat4("view", ctx.cameraView);
    geomShader->setMat4("projection", ctx.cameraProjection);
    geomShader->setVec3("cameraPos", ctx.cameraPos);


    for (auto& [entityID, mr] : meshRenderers.All()) {

        if (!transforms.Has(entityID)) continue;
        if (!meshFilters.Has(entityID)) continue;


        const TransformComponent& t = transforms.Get(entityID);
        const MeshFilter& mf = meshFilters.Get(entityID);


        if (mf.HasPendingSubmesh()) continue;
        if (mr.inst.baseMaterial.isNull() || !mr.render) continue;

        glm::mat4 model = t.worldMatrix;
        geomShader->setMat4("model", model);

        Mesh* mesh = nullptr;

        if (!mf.meshID.isNull()) {
            mesh = assetManager.GetSubmesh(mf.meshID);
        }

        // Animation
        geomShader->setBool("useSkeleton", false);


        if (animations.Has(mf.rootParent)) {
            const AnimationComponent& anim = animations.Get(mf.rootParent);

            if (mesh && anim.currentAnimationID != UUID::Null && anim.finalBoneMatrices.size())
            {
                for (int i = 0; i < mesh->bonePalette.size(); i++)
                {
                    int globalID = mesh->bonePalette[i];

                    geomShader->setMat4(
                        "finalBonesMatrices[" + std::to_string(i) + "]",
                        anim.finalBoneMatrices[globalID]
                    );
                }

                geomShader->setBool("useSkeleton", true);

            }
            else {
                geomShader->setBool("useSkeleton", false);

            }

        }
            
      

        Material* mat = assetManager.GetMaterial(mr.inst.baseMaterial);

        if (!mat) continue;

        const MaterialInstance& inst = mr.inst;
        const ResolvedMaterial& finalMat = ResolveMaterial(*mat, inst);

        geomShader->setVec3("material.albedo", finalMat.albedo);
        geomShader->setFloat("material.metallic", finalMat.metallic);
        geomShader->setFloat("material.roughness", finalMat.roughness);
        geomShader->setFloat("material.ao", finalMat.ao);
        geomShader->setFloat("material.normalStrength", finalMat.normalStrength);


        bindTexture(
            *geomShader,
            assetManager,
            finalMat.map_albedo,
            inst.use_map_albedo,
            "material.hasAlbedoMap",
            "material.albedoMap",
            GL_TEXTURE0 + static_cast<unsigned int>(TextureUnit::Albedo)
        );

        bindTexture(
            *geomShader,
            assetManager,
            finalMat.map_normal,
            inst.use_map_normal,
            "material.hasNormalMap",
            "material.normalMap",
            GL_TEXTURE0 + static_cast<unsigned int>(TextureUnit::Normal)
        );

        bindTexture(
            *geomShader,
            assetManager,
            finalMat.map_ao,
            inst.use_map_ao,
            "material.hasAOMap",
            "material.aoMap",
            GL_TEXTURE0 + static_cast<unsigned int>(TextureUnit::AO)
        );

        bindTexture(
            *geomShader,
            assetManager,
            finalMat.map_metallic,
            inst.use_map_metallic,
            "material.hasMetallicMap",
            "material.metallicMap",
            GL_TEXTURE0 + static_cast<unsigned int>(TextureUnit::Metallic)
        );

        bindTexture(
            *geomShader,
            assetManager,
            finalMat.map_roughness,
            inst.use_map_roughness,
            "material.hasRoughnessMap",
            "material.roughnessMap",
            GL_TEXTURE0 + static_cast<unsigned int>(TextureUnit::Roughness)
        );

        bindTexture(
            *geomShader,
            assetManager,
            finalMat.map_metallicRoughness,
            inst.use_map_metallicRoughness,
            "material.hasMetallicRoughnessMap",
            "material.metallicRoughnessMap",
            GL_TEXTURE0 + static_cast<unsigned int>(TextureUnit::MetallicRoughness)
        );


        if (mesh) mesh->draw();


    }

    geomShader->unuse();
}

void DeferredRenderer::RenderLighting(const RenderContext& ctx, const Framebuffer& gBuffer) {
    GLSLProgram* shader = assetManager.getShader(ShaderRegistry::DEFERRED_PBR);

    const Scene* activeScene = ctx.scene;

    auto& lightComponents = activeScene->Lights();
    auto& transforms = activeScene->Transforms();

    shader->use();

    shader->setInt("gPosition", 0);
    shader->setInt("gNormal", 1);
    shader->setInt("gAlbedo", 2);
    shader->setInt("gMaterial", 3);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(
        GL_TEXTURE_2D,
        gBuffer.GetColorAttachment(0)
    );

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(
        GL_TEXTURE_2D,
        gBuffer.GetColorAttachment(1)
    );

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(
        GL_TEXTURE_2D,
        gBuffer.GetColorAttachment(2)
    );

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(
        GL_TEXTURE_2D,
        gBuffer.GetColorAttachment(3)
    );

    // LIGHTING
    uint32_t lightNum = 0;

    for (const auto& [id, light] : lightComponents.All())
    {
        if (!transforms.Has(id))
            continue;

        if (lightNum >= MAX_LIGHTS)
            break;

        const Light& l = light;

        // ---------------- Common ----------------
        shader->setInt(
            "lightTypes[" + std::to_string(lightNum) + "]",
            static_cast<int>(l.type)
        );

        shader->setVec3(
            "lightColors[" + std::to_string(lightNum) + "]",
            l.color
        );

        shader->setFloat(
            "lightIntensities[" + std::to_string(lightNum) + "]",
            l.intensity
        );

        shader->setBool(
            "lightCastShadow[" + std::to_string(lightNum) + "]",
            light.castShadow
        );

        // ---------------- Point & Spot ----------------
        shader->setFloat(
            "lightRanges[" + std::to_string(lightNum) + "]",
            l.range
        );

        // ---------------- Spot only ----------------
        shader->setFloat(
            "lightInnerAngles[" + std::to_string(lightNum) + "]",
            glm::cos(glm::radians(l.innerAngle))
        );

        shader->setFloat(
            "lightOuterAngles[" + std::to_string(lightNum) + "]",
            glm::cos(glm::radians(l.outerAngle))
        );

        const TransformComponent& t = transforms.Get(id);

        glm::vec3 position = t.GetWorldPosition();
        glm::quat rotation = t.GetWorldRotation();

        shader->setVec3(
            "lightPositions[" + std::to_string(lightNum) + "]",
            position
        );

        glm::vec3 direction = glm::normalize(rotation * glm::vec3(0.0f, 0.0f, -1.0f));

        shader->setVec3(
            "lightDirections[" + std::to_string(lightNum) + "]",
            direction
        );

        ++lightNum;


    }

    shader->setInt("lightCount", lightNum);

    // IBL

    shader->setInt("irradianceMap", static_cast<unsigned int>(TextureUnit::Irradiance));
    shader->setInt("prefilterMap", static_cast<unsigned int>(TextureUnit::Prefilter));
    shader->setInt("brdfLUT", static_cast<unsigned int>(TextureUnit::BRDF_LUT));
    shader->setFloat("envIntensity", ctx.envIntensity);
    shader->setVec3("envTint", ctx.envTint);
    shader->setMat3("envRotation", ctx.envRotation);

    glActiveTexture(GL_TEXTURE0 + static_cast<unsigned int>(TextureUnit::Irradiance));
    glBindTexture(GL_TEXTURE_CUBE_MAP, ctx.irradianceMap.id);

    glActiveTexture(GL_TEXTURE0 + static_cast<unsigned int>(TextureUnit::Prefilter));
    glBindTexture(GL_TEXTURE_CUBE_MAP, ctx.prefilterMap.id);

    glActiveTexture(GL_TEXTURE0 + static_cast<unsigned int>(TextureUnit::BRDF_LUT));
    glBindTexture(GL_TEXTURE_2D, ctx.brdfLUTMap.id);

    shader->setInt("shadowMap", static_cast<unsigned int>(TextureUnit::Shadow2D));
    shader->setInt("shadowCubeMap", static_cast<unsigned int>(TextureUnit::ShadowCube));

    if (activeScene->GetDirectionalShadowCaster() != UUID::Null
        && transforms.Has(activeScene->GetDirectionalShadowCaster())
        )
    {
        bindShadowMapUniforms(
            *shader,
            *ctx.shadowMap,
            transforms.Get(activeScene->GetDirectionalShadowCaster()),
            ctx.cameraPos
        );
    }

    if (activeScene->GetPointShadowCaster() != UUID::Null
        && transforms.Has(activeScene->GetPointShadowCaster())
        )
    {

        bindPointShadowUniforms(
            *shader,
            *ctx.shadowCubeMap
        );
    }

    shader->setVec3("cameraPos", ctx.cameraPos);

    fullscreenQuad.draw();

    shader->unuse();

}


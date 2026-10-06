#include "DeferredRenderer.h"

using namespace Lengine;


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



void DeferredRenderer::bindShadowMapUniforms(
    GLSLProgram& shader,
    ShadowMap& shadowMap,
    const TransformComponent& lightTransform,
    const glm::vec3& camPos
) {

    glm::mat4 lightSpaceMat = shadowMap.lightSpaceMat;

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

    shader.setFloat("pointShadowFarPlane", shadowCubeMap.getFarPlane());
}

void DeferredRenderer::RenderGeometry(const RenderContext& ctx)
{
    const Scene* scene = ctx.scene;
    const Registry& registry = scene->GetRegistry();
    auto geomShader = assetManager.getShader(ShaderRegistry::GEOMETRY);

    opaqueQueue.Clear();
    transparentQueue.Clear();

    const auto& mrDense = registry.meshRenderers.GetDense();
    const auto& mrEntities = registry.meshRenderers.GetEntities();

    // Camera forward for depth calculation
    glm::vec3 camForward = glm::normalize(
        glm::vec3(glm::inverse(ctx.cameraView) * glm::vec4(0, 0, -1, 0))
    );

    for (size_t i = 0; i < mrDense.size(); ++i)
    {
        const MeshRenderer& mr = mrDense[i];
        const Entity        entity = mrEntities[i];

        if (!mr.render)                                          continue;
        if (mr.inst.baseMaterial.isNull())                       continue;
        if (!registry.HasComponent<TransformComponent>(entity))  continue;
        if (!registry.HasComponent<MeshFilter>(entity))          continue;

        const TransformComponent& t = registry.GetComponent<TransformComponent>(entity);
        const MeshFilter& mf = registry.GetComponent<MeshFilter>(entity);

        if (mf.HasPendingSubmesh()) continue;

        Mesh* mesh = mf.meshID.isNull()
            ? nullptr
            : assetManager.GetSubmesh(mf.meshID);
        if (!mesh) continue;

        Material* mat = assetManager.GetMaterial(mr.inst.baseMaterial);
        if (!mat) continue;

        // Build the item
        RenderItem item;
        item.mesh = mesh;
        item.entity = entity;
        item.modelMatrix = t.worldMatrix;
        item.material = ResolveMaterial(*mat, mr.inst);

        // Skeleton
        if (registry.HasComponent<AnimationComponent>(mf.rootParent))
        {
            const AnimationComponent& anim =
                registry.GetComponent<AnimationComponent>(mf.rootParent);

            if (!anim.finalBoneMatrices.empty()
                && !mesh->bonePalette.empty())
            {

                item.hasSkeleton = true;
                item.boneMatrices = &anim.finalBoneMatrices;
                item.bonePalette = &mesh->bonePalette;
            }
        }

        // Sort key — depth from camera (front = small value = drawn first)
        glm::vec3 worldCenter = glm::vec3(t.worldMatrix * glm::vec4(mesh->localCenter, 1.0f));
        float depth = glm::dot(camForward, worldCenter - ctx.cameraPos);

        if (item.material.isTransparent)
        {
            item.sortKey = BuildDepthSortKey(depth, true);
            transparentQueue.Submit(std::move(item));
        }
        else {
            item.sortKey = BuildDepthSortKey(depth, false);
            opaqueQueue.Submit(std::move(item));
        }
       
    }
     
    opaqueQueue.Sort();
    transparentQueue.Sort();

    geometryCommandBuffer.Clear();

    GeometryQueueFlusher::Flush(
        opaqueQueue,
        geomShader.get(),
        ctx,
        assetManager,
        geometryCommandBuffer);

    geometryCommandBuffer.Execute();

}

void DeferredRenderer::RenderLighting(const RenderContext& ctx, const Framebuffer& gBuffer)
{
    const Scene* activeScene = ctx.scene;
    const Registry& registry = activeScene->GetRegistry();

    auto shader = assetManager.getShader(ShaderRegistry::DEFERRED_PBR);
    shader->use();

    shader->setInt("gPosition", 0);
    shader->setInt("gNormal", 1);
    shader->setInt("gAlbedo", 2);
    shader->setInt("gMaterial", 3);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gBuffer.GetColorAttachment(0));

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gBuffer.GetColorAttachment(1));

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gBuffer.GetColorAttachment(2));

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, gBuffer.GetColorAttachment(3));

    uint32_t lightNum = 0;

    const auto& lightDense = registry.lights.GetDense();
    const auto& lightEntities = registry.lights.GetEntities();

    for (size_t i = 0; i < lightDense.size() && lightNum < MAX_LIGHTS; ++i)
    {
        const Entity id = lightEntities[i];
        const Light& l = lightDense[i];

        if (!registry.HasComponent<TransformComponent>(id))
            continue;

        shader->setInt("lightTypes[" + std::to_string(lightNum) + "]", static_cast<int>(l.type));
        shader->setVec3("lightColors[" + std::to_string(lightNum) + "]", l.color);
        shader->setFloat("lightIntensities[" + std::to_string(lightNum) + "]", l.intensity);
        shader->setBool("lightCastShadow[" + std::to_string(lightNum) + "]", l.castShadow);
        shader->setFloat("lightRanges[" + std::to_string(lightNum) + "]", l.range);
        shader->setFloat("lightInnerAngles[" + std::to_string(lightNum) + "]", glm::cos(glm::radians(l.innerAngle)));
        shader->setFloat("lightOuterAngles[" + std::to_string(lightNum) + "]", glm::cos(glm::radians(l.outerAngle)));

        const TransformComponent& t = registry.GetComponent<TransformComponent>(id);

        shader->setVec3("lightPositions[" + std::to_string(lightNum) + "]", t.GetWorldPosition());
        shader->setVec3("lightDirections[" + std::to_string(lightNum) + "]",
            glm::normalize(t.GetWorldRotation() * glm::vec3(0.0f, 0.0f, -1.0f)));

        ++lightNum;
    }

    shader->setInt("lightCount", lightNum);
    shader->setInt("irradianceMap", static_cast<unsigned int>(TextureUnit::Irradiance));
    shader->setInt("prefilterMap", static_cast<unsigned int>(TextureUnit::Prefilter));
    shader->setInt("brdfLUT", static_cast<unsigned int>(TextureUnit::BRDF_LUT));
    shader->setFloat("envIntensity", ctx.envIntensity);
    shader->setVec3("envTint", ctx.envTint);
    shader->setMat3("envRotation", ctx.envRotation);
    shader->setInt("shadowMap", static_cast<unsigned int>(TextureUnit::Shadow2D));
    shader->setInt("shadowCubeMap", static_cast<unsigned int>(TextureUnit::ShadowCube));

    float shadowTexelWorldSize = (ctx.shadowContext.shadowMap->shadowExtent * 2.0f) / static_cast<float>((ctx.shadowContext.shadowMap->SHADOW_RES));
    shader->setFloat("shadowTexelWorldSize", shadowTexelWorldSize);
    shader->setFloat("shadowNearPlane", (ctx.shadowContext.shadowMap->shadowNear));
    shader->setFloat("shadowFarPlane", (ctx.shadowContext.shadowMap->shadowFar));
    shader->setFloat("pointShadowFarplane", (ctx.shadowContext.shadowCubeMap->farPlane));

    glActiveTexture(GL_TEXTURE0 + static_cast<unsigned int>(TextureUnit::Irradiance));
    glBindTexture(GL_TEXTURE_CUBE_MAP, ctx.irradianceMap.id);

    glActiveTexture(GL_TEXTURE0 + static_cast<unsigned int>(TextureUnit::Prefilter));
    glBindTexture(GL_TEXTURE_CUBE_MAP, ctx.prefilterMap.id);

    glActiveTexture(GL_TEXTURE0 + static_cast<unsigned int>(TextureUnit::BRDF_LUT));
    glBindTexture(GL_TEXTURE_2D, ctx.brdfLUTMap.id);

    if (activeScene->GetDirectionalShadowCaster() != UUID::Null
        && registry.HasComponent<TransformComponent>(activeScene->GetDirectionalShadowCaster()))
    {
        bindShadowMapUniforms(
            *shader,
            *ctx.shadowContext.shadowMap,
            registry.GetComponent<TransformComponent>(activeScene->GetDirectionalShadowCaster()),
            ctx.cameraPos
        );
    }

    if (activeScene->GetPointShadowCaster() != UUID::Null
        && registry.HasComponent<TransformComponent>(activeScene->GetPointShadowCaster()))
    {
        bindPointShadowUniforms(*shader, *ctx.shadowContext.shadowCubeMap);
    }

    shader->setVec3("cameraPos", ctx.cameraPos);

    fullscreenQuad.draw();
    shader->unuse();
}
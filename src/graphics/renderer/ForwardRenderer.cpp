#include "ForwardRenderer.h"

#include "../logging/LogBuffer.h"
using namespace Lengine;



void ForwardRenderer::RenderScene(const RenderContext& ctx)
{
    CollectAndSort(ctx);
    FlushOpaqueQueue(ctx);
    FlushTransparentQueue(ctx);
}


void ForwardRenderer::bindTexture(
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



void ForwardRenderer::bindShadowMapUniforms(
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

void ForwardRenderer::bindPointShadowUniforms(
    GLSLProgram& shader,
    ShadowCubeMap& shadowCubeMap
) {
    glActiveTexture(GL_TEXTURE0 + static_cast<unsigned int>(TextureUnit::ShadowCube));
    glBindTexture(GL_TEXTURE_CUBE_MAP, shadowCubeMap.getDepthCubeMap());
    shader.setInt("shadowCubeMap", static_cast<unsigned int>(TextureUnit::ShadowCube));

}


void ForwardRenderer::brdfPass(const RenderContext& ctx, std::shared_ptr<GLSLProgram> pbrShader) {

    pbrShader->setInt("irradianceMap", static_cast<unsigned int>(TextureUnit::Irradiance));
    pbrShader->setInt("prefilterMap", static_cast<unsigned int>(TextureUnit::Prefilter));
    pbrShader->setInt("brdfLUT", static_cast<unsigned int>(TextureUnit::BRDF_LUT));
    pbrShader->setFloat("envIntensity", ctx.envIntensity);
    pbrShader->setVec3("envTint", ctx.envTint);
    pbrShader->setMat3("envRotation", ctx.envRotation);


    glActiveTexture(GL_TEXTURE0 + static_cast<unsigned int>(TextureUnit::Irradiance));
    glBindTexture(GL_TEXTURE_CUBE_MAP, ctx.irradianceMap.id);

    glActiveTexture(GL_TEXTURE0 + static_cast<unsigned int>(TextureUnit::Prefilter));
    glBindTexture(GL_TEXTURE_CUBE_MAP, ctx.prefilterMap.id);

    glActiveTexture(GL_TEXTURE0 + static_cast<unsigned int>(TextureUnit::BRDF_LUT));
    glBindTexture(GL_TEXTURE_2D, ctx.brdfLUTMap.id);
}

void ForwardRenderer::shadowMapPass(const RenderContext& ctx, std::shared_ptr<GLSLProgram> pbrShader) {

    const auto& registry = ctx.scene->GetRegistry();

    pbrShader->setInt("shadowMap", static_cast<unsigned int>(TextureUnit::Shadow2D));
    pbrShader->setInt("shadowCubeMap", static_cast<unsigned int>(TextureUnit::ShadowCube));

    float shadowTexelWorldSize = (ctx.shadowContext.shadowMap->shadowExtent * 2.0f) / static_cast<float>((ctx.shadowContext.shadowMap->SHADOW_RES));
    pbrShader->setFloat("shadowTexelWorldSize", shadowTexelWorldSize);
    pbrShader->setFloat("shadowNearPlane", (ctx.shadowContext.shadowMap->shadowNear));
    pbrShader->setFloat("shadowFarPlane", (ctx.shadowContext.shadowMap->shadowFar));
    pbrShader->setFloat("farPlaneCubeMap", (ctx.shadowContext.shadowCubeMap->farPlane));



    if (ctx.scene->GetDirectionalShadowCaster() != UUID::Null
        && registry.HasComponent<TransformComponent>(ctx.scene->GetDirectionalShadowCaster()))
    {
        bindShadowMapUniforms(
            *pbrShader,
            *ctx.shadowContext.shadowMap,
            registry.GetComponent<TransformComponent>(ctx.scene->GetDirectionalShadowCaster()),
            ctx.cameraPos
        );
    }

    if (ctx.scene->GetPointShadowCaster() != UUID::Null
        && registry.HasComponent<TransformComponent>(ctx.scene->GetPointShadowCaster()))
    {
        bindPointShadowUniforms(*pbrShader, *ctx.shadowContext.shadowCubeMap);
    }
}

void ForwardRenderer::lightingPass(const RenderContext& ctx, std::shared_ptr<GLSLProgram> pbrShader) {

    const auto& registry = ctx.scene->GetRegistry();

    uint32_t lightNum = 0;

    const auto& lightDense = registry.lights.GetDense();
    const auto& lightEntities = registry.lights.GetEntities();

    for (size_t i = 0; i < lightDense.size() && lightNum < MAX_LIGHTS; ++i)
    {
        const Entity id = lightEntities[i];
        const Light& l = lightDense[i];

        if (!registry.HasComponent<TransformComponent>(id))
            continue;

        pbrShader->setInt("lightTypes[" + std::to_string(lightNum) + "]", static_cast<int>(l.type));
        pbrShader->setVec3("lightColors[" + std::to_string(lightNum) + "]", l.color);
        pbrShader->setFloat("lightIntensities[" + std::to_string(lightNum) + "]", l.intensity);
        pbrShader->setBool("lightCastShadow[" + std::to_string(lightNum) + "]", l.castShadow);
        pbrShader->setFloat("lightRanges[" + std::to_string(lightNum) + "]", l.range);
        pbrShader->setFloat("lightInnerAngles[" + std::to_string(lightNum) + "]", glm::cos(glm::radians(l.innerAngle)));
        pbrShader->setFloat("lightOuterAngles[" + std::to_string(lightNum) + "]", glm::cos(glm::radians(l.outerAngle)));

        const TransformComponent& t = registry.GetComponent<TransformComponent>(id);

        pbrShader->setVec3("lightPositions[" + std::to_string(lightNum) + "]", t.GetWorldPosition());
        pbrShader->setVec3("lightDirections[" + std::to_string(lightNum) + "]",
            glm::normalize(t.GetWorldRotation() * glm::vec3(0.0f, 0.0f, -1.0f)));

        ++lightNum;
    }

    pbrShader->setInt("lightCount", lightNum);
}

void ForwardRenderer::CollectAndSort(const RenderContext& ctx)
{
    const Scene* scene = ctx.scene;
    const Registry& registry = scene->GetRegistry();

    opaqueQueue.Clear();
    transparentQueue.Clear();

    glm::vec3 camForward = glm::normalize(
        glm::vec3(glm::inverse(ctx.cameraView) * glm::vec4(0, 0, -1, 0))
    );

    const auto& mrDense = registry.meshRenderers.GetDense();
    const auto& mrEntities = registry.meshRenderers.GetEntities();

    for (size_t i = 0; i < mrDense.size(); ++i)
    {
        const MeshRenderer& mr = mrDense[i];
        const Entity        entity = mrEntities[i];


        if (!mr.render)                                         continue;
        if (mr.inst.baseMaterial.isNull())                      continue;
        if (!registry.HasComponent<TransformComponent>(entity)) continue;
        if (!registry.HasComponent<MeshFilter>(entity))         continue;


        const TransformComponent& t = registry.GetComponent<TransformComponent>(entity);
        const MeshFilter& mf = registry.GetComponent<MeshFilter>(entity);
        if (mf.HasPendingSubmesh()) continue;

        Mesh* mesh = mf.meshID.isNull() ? nullptr : assetManager.GetSubmesh(mf.meshID);
        if (!mesh) continue;

        Material* mat = assetManager.GetMaterial(mr.inst.baseMaterial);
        if (!mat) continue;

        RenderItem item;
        item.mesh = mesh;
        item.entity = entity;
        item.modelMatrix = t.worldMatrix;
        item.material = ResolveMaterial(*mat, mr.inst);

        
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

        glm::vec3 worldCenter = glm::vec3(
            t.worldMatrix * glm::vec4(mesh->localCenter, 1.0f)
        );
        float depth = glm::dot(camForward, worldCenter - ctx.cameraPos);

        if (item.material.isTransparent)
        {
            item.sortKey = BuildDepthSortKey(depth, true);
            transparentQueue.Submit(std::move(item));
        }
        else
        {
            item.sortKey = BuildDepthSortKey(depth, false);
            opaqueQueue.Submit(std::move(item));
        }
    }

    opaqueQueue.Sort();
    transparentQueue.Sort();
}

void ForwardRenderer::FlushOpaqueQueue(const RenderContext& ctx)
{
    auto pbrShader = assetManager.getShader(ShaderRegistry::UNIVERSAL_PBR);

    pbrShader->use();
    brdfPass(ctx, pbrShader);
    shadowMapPass(ctx, pbrShader);
    lightingPass(ctx, pbrShader);
    pbrShader->unuse();

    forwardCommandBuffer.Clear();
    GeometryQueueFlusher::Flush(
        opaqueQueue,
        pbrShader.get(),
        ctx,
        assetManager,
        forwardCommandBuffer
    );
    forwardCommandBuffer.Execute();
}

void ForwardRenderer::FlushTransparentQueue(
    const RenderContext& ctx,
    bool ueseExternalQueue,
    const RenderQueue& externalQueue
)
{
    
    auto& finalQueue = transparentQueue;

    if (ueseExternalQueue) {
        finalQueue = externalQueue;
    }

    auto pbrShader = assetManager.getShader(ShaderRegistry::UNIVERSAL_PBR);

    pbrShader->use();
    brdfPass(ctx, pbrShader);
    shadowMapPass(ctx, pbrShader);
    lightingPass(ctx, pbrShader);
    pbrShader->unuse();

    forwardCommandBuffer.Clear();
    forwardCommandBuffer.Record<EnableBlendCommand>();
    GeometryQueueFlusher::Flush(
        finalQueue,
        pbrShader.get(),
        ctx,
        assetManager,
        forwardCommandBuffer
    );
    forwardCommandBuffer.Record<DisableBlendCommand>();
    forwardCommandBuffer.Execute();
}


void ForwardRenderer::RenderScene_debug(
    const RenderContext& ctx
) {


}


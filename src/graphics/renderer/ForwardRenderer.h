#pragma once

#include "../scene/Scene.h"
#include "../scene/SceneManager.h"
#include "../graphics/opengl/GLSLProgram.h"
#include "../graphics/camera/Camera3d.h"
#include "../scene/components/Light.h"
#include "../graphics/renderer/IRenderer.h"
#include "../graphics/shadowMaps/shadowMap.h"
#include "../graphics/shadowMaps/shadowCubeMap.h"
#include "../resources/TextureCache.h"

#include "../resources/AssetManager.h"

namespace Lengine {

    struct RenderFlags {
        bool entitySelected = false;
        bool entityDragged = false;
    };


    class ForwardRenderer : public IRenderer {
    public:
        ForwardRenderer(
            AssetManager& assetmgr

        ) :
            assetManager(assetmgr)
        {
        }

        void Render(
            const RenderContext& ctx
        ) override
        {
            
            if (IRenderer::enableDebugView) RenderScene_debug(ctx);
            else RenderScene_pbr(ctx);
        }

   
    private:
        AssetManager& assetManager;

        float nearPlane = 0.1f;
        float farPlane = 1000.5f;

        ResolvedMaterial resolvePBRMaterial(
            const Material& baseMaterial,
            const MaterialInstance& inst
        );



        void RenderScene_pbr(
            const RenderContext& ctx

        );

        void RenderScene_debug(
            const RenderContext& ctx
        );


        void bindShadowMapUniforms(
            GLSLProgram& shader,
            ShadowMap& shadowMap,
            const TransformComponent& lightTransform,
            const glm::vec3& camPos
        );
        void bindPointShadowUniforms(
            GLSLProgram& shader,
            ShadowCubeMap& shadowCubeMap
        );

        void bindCameraUniforms(
            GLSLProgram& shader,
            const glm::mat4& model,
            Camera3d& editorCamera
        );


        void bindPBRLights(
            GLSLProgram& shader,
            const std::vector<Light>& lights
        );

        void bindPBRMaterial(
            GLSLProgram& shader,
            const ResolvedMaterial& mat
        );


        void bindTexture(
            GLSLProgram& shader,
            AssetManager& assetManager,
            const UUID& texID,
            const bool  use,
            const char* hasUniform,
            const char* samplerUniform,
            GLenum textureUnit
        );

        void drawSubMesh(
            Mesh& sm,
            GLSLProgram& shader
        );



    };
}


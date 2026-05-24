#pragma once

#include "../graphics/renderer/PostProcess/PostProcessing.h"
#include "../graphics/geometry/HDREnvironment.h"
#include "../graphics/frameBuffers/FrameBuffer.h"
#include "../graphics/renderer/ForwardRenderer.h"
#include "../graphics/renderer/DeferredRenderer.h"


namespace Lengine {


    class RenderPass
    {
    public:
        virtual ~RenderPass() = default;
        virtual void Execute(RenderContext& ctx) = 0;
    };

    class ShadowPass : public RenderPass
    {
    public:
        ShadowPass(ShadowCubeMap& shadowcube_ , ShadowMap& shadow_, AssetManager& asset_) :
            shadowCubemap(shadowcube_),
            shadowMap(shadow_),
            assetManager(asset_) 
        {
        }
            
        void Execute(RenderContext& ctx) override
        {

            glCullFace(GL_FRONT);
            shadowMap.renderDepthMap(
                ctx.scene->getEntities(),
                ctx.scene->GetRegistry().transforms,
                ctx.scene->GetRegistry().meshFilters,
                ctx.scene->GetDirectionalShadowCaster(),
                assetManager,
                ctx.cameraPos
            );
            glCullFace(GL_BACK);

            shadowCubemap.renderDepthCubeMap(
                ctx.scene->getEntities(),
                ctx.scene->GetRegistry().transforms,
                ctx.scene->GetRegistry().meshFilters,
                ctx.scene->GetPointShadowCaster(),
                assetManager
            );

            glViewport(
                0, 0,
                ctx.settings->resolution_X,
                ctx.settings->resolution_Y
            );
           
        }
    private:
        ShadowCubeMap& shadowCubemap;
        ShadowMap& shadowMap;
        AssetManager& assetManager;
    };

    class ForwardPass : public RenderPass
    {
    public:
        ForwardPass(ForwardRenderer& renderer,
            Framebuffer& target)
            : renderer(renderer), target(target) {
        }

        void Execute(RenderContext& ctx) override
        {
            target.Bind();
            glClearColor(0, 0, 0, 1);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            renderer.Render(ctx);

            target.Unbind();
        }

    private:
        ForwardRenderer& renderer;
        Framebuffer& target;
    };

    class GeometryPass : public RenderPass
    {
    public:
        GeometryPass(DeferredRenderer& renderer,
            Framebuffer& target)
            : renderer(renderer), target(target) {}

        void Execute(RenderContext& ctx) override
        {
            target.Bind();

            glClearColor(0, 0, 0, 1);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            renderer.RenderGeometry(ctx);

            target.Unbind();
        }

    private:
        DeferredRenderer& renderer;
        Framebuffer& target;
    };

    class DeferredLightingPass : public RenderPass
    {
    public:
        DeferredLightingPass(
            DeferredRenderer& renderer,
            Framebuffer& target,
            Framebuffer& gBuffer
            )
            : renderer(renderer),
            target(target),
            gBuffer(gBuffer)
        {}

        void Execute(RenderContext& ctx) override
        {
            target.Bind();
            glClear(GL_COLOR_BUFFER_BIT);

            glDisable(GL_DEPTH_TEST);
            glDepthMask(GL_FALSE);

            renderer.RenderLighting(ctx, gBuffer);

            glDepthMask(GL_TRUE);
            glEnable(GL_DEPTH_TEST);

            target.Unbind();

        }

    private:
        DeferredRenderer& renderer;
        Framebuffer& target;
        Framebuffer& gBuffer;
    };

    class DepthCopyPass : public RenderPass
    {
    public:
        DepthCopyPass(
            Framebuffer& src,
            Framebuffer& dst)
            : source(src), destination(dst)
        {}

        void Execute(RenderContext& ctx) override
        {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, source.GetID());
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, destination.GetID());

            glBlitFramebuffer(
                0, 0,
                source.GetWidth(),
                source.GetHeight(),

                0, 0,
                destination.GetWidth(),
                destination.GetHeight(),

                GL_DEPTH_BUFFER_BIT,
                GL_NEAREST
            );

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

    private:
        Framebuffer& source;
        Framebuffer& destination;
    };

    class ResolvePass : public RenderPass
    {
    public:
        ResolvePass(Framebuffer& src, Framebuffer& dst)
            : source(src), destination(dst) {
        }

        void Execute(RenderContext& ctx) override
        {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, source.GetID());
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, destination.GetID());

           
            glBlitFramebuffer(
                0, 0, source.GetWidth(), source.GetHeight(),
                0, 0, destination.GetWidth(), destination.GetHeight(),
                GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT,
                GL_NEAREST
            );

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

    private:
        Framebuffer& source;
        Framebuffer& destination;
    };


    class ToneMapPass : public RenderPass
    {
    public:
        ToneMapPass(
            PostProcessing& postProcess_,
            Framebuffer& mainBuffer,
            Framebuffer& hdrBuffer_
            ) : 
            postProcess(postProcess_),
            mainBuffer(mainBuffer),
            hdrBuffer(hdrBuffer_)       
        {
            //postProcess.InitToneMappingResources();

        }
        void Execute(RenderContext& ctx) override
        {
            mainBuffer.Bind();

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, hdrBuffer.GetColorAttachment(0));

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, postProcess.getBloomColorBuffer());

            glClear(GL_COLOR_BUFFER_BIT);
            glDisable(GL_DEPTH_TEST);
            glDepthMask(GL_FALSE);

            postProcess.renderToneMapping(ctx.settings->enableBloom, ctx.settings->exposure);

            glDepthMask(GL_TRUE);
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LESS);   
            mainBuffer.Unbind();

            // this prevents from depth data loss 
            // which is needed for editor overlays
            glBindFramebuffer(GL_READ_FRAMEBUFFER, hdrBuffer.GetID());
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mainBuffer.GetID());

            glBlitFramebuffer(
                0, 0, ctx.settings->resolution_X, ctx.settings->resolution_Y,
                0, 0, ctx.settings->resolution_X, ctx.settings->resolution_Y,
                GL_DEPTH_BUFFER_BIT,
                GL_NEAREST
            );

            glBindFramebuffer(GL_FRAMEBUFFER, 0);


        }
    private:
        PostProcessing& postProcess;
        Framebuffer& mainBuffer;
        Framebuffer& hdrBuffer;
    };

    class BloomPass : public RenderPass
    {
    public:
        BloomPass(PostProcessing& postProcess_, Framebuffer& hdrBuffer, const uint32_t width, const uint32_t height) :
            postProcess(postProcess_),
            hdrBuffer(hdrBuffer)

        {
            postProcess.InitBloom(width, height);
        }
        void Execute(RenderContext& ctx) override
        {
            postProcess.renderBloom(hdrBuffer.GetColorAttachment(1), ctx.settings->bloomBlur);

        }
    private:
        PostProcessing& postProcess;
        Framebuffer& hdrBuffer;
 
    };

    class SkyboxPass : public RenderPass
    {
    public:
        SkyboxPass(Framebuffer& target_, HDREnvironment& hdrSykybox_, std::string texPath_, uint32_t texRes_) :
            target(target_),
            hdrSkybox(hdrSykybox_)
        {

        }
        void Execute(RenderContext& ctx) override
        {
            target.Bind();
            hdrSkybox.Render(ctx.cameraView, ctx.cameraProjection);
            target.Unbind();
        }
    private:
        Framebuffer& target;
        HDREnvironment& hdrSkybox;

    };

   


    class RenderGraph
    {
    public:
        RenderGraph() = default;
        ~RenderGraph() = default;

        void AddPass(std::unique_ptr<RenderPass> pass)
        {
            passes.push_back(std::move(pass));
        }

        void Clear()
        {
            passes.clear();
        }

        void Execute(RenderContext& context)
        {
            for (auto& pass : passes)
            {
                pass->Execute(context);
            }
        }

    private:
        std::vector<std::unique_ptr<RenderPass>> passes;
    };


    class RenderPipeline
    {
    public:
        RenderPipeline(AssetManager& assetManager_) :
            assetManager(assetManager_), 
            forwardRenderer(assetManager_),
            deferredRenderer(assetManager_),
            shadowMap(1024),
            shadowCubemap(1024)
        {}
        void Init();
        void Resize(uint32_t width, uint32_t height);

        void Render(RenderContext& ctx);
        Framebuffer& GetFinalFramebuffer();
       
        void SetRenderSettings(const RenderSettings& settings);
        RenderSettings& GetRenderSettings();

        uint32_t GetFinalImage() const
        {
            return mainFramebuffer->GetColorAttachment(0);
        }

        HDREnvironment& GetHDRSkybox() 
        {
            return hdrSkybox;
        }

        uint32_t GetGbufferPosition() const {
            return gBuffer->GetColorAttachment(0);
        }

        uint32_t GetGbufferNormal() const {
            return gBuffer->GetColorAttachment(1);
        }

        uint32_t GetGbufferAlbedo() const {
            return gBuffer->GetColorAttachment(2);
        }

        uint32_t GetGbufferMR() const {
            return gBuffer->GetColorAttachment(3);
        }

    private:
        AssetManager& assetManager;

        ForwardRenderer forwardRenderer;
        DeferredRenderer deferredRenderer;

        PostProcessing postProcess;
        HDREnvironment hdrSkybox;
        
        ShadowMap shadowMap;
        ShadowCubeMap shadowCubemap;

        std::unique_ptr<Framebuffer> mainFramebuffer;
        std::unique_ptr<Framebuffer> hdrFramebuffer;
        std::unique_ptr<Framebuffer> msaaFramebuffer;
        std::unique_ptr<Framebuffer> gBuffer;

        RenderGraph renderGraph;

        void BuildGraph();
        void RecreateFramebuffers();
        void PostProcess();

        RenderSettings renderSettings;

        uint32_t viewportWidth = 1280;
        uint32_t viewportHeight = 720;
    };

}


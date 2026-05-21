#include "RenderPipeline.h"

using namespace Lengine;

void RenderPipeline::Init() {
    // Depth
    glEnable(GL_DEPTH_TEST);

    // Face culling
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // Blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // MSAA
    glEnable(GL_MULTISAMPLE);

    GLTexture hdrTex = ImageLoader::LoadHDRTexture(Paths::Textures + "Hdr/" + "citrus_orchard_road_puresky_2k.hdr");
    hdrSkybox.Init(512);
    hdrSkybox.SetHDRTexture(hdrTex);

    postProcess.InitHDRShaders();


}

void RenderPipeline::RecreateFramebuffers()
{
    // Destroy old ones automatically by resetting unique_ptr
    mainFramebuffer.reset();
    hdrFramebuffer.reset();
    msaaFramebuffer.reset();


    FramebufferSpecification mainBufferSpec;
    mainBufferSpec.width = renderSettings.resolution_X;
    mainBufferSpec.height = renderSettings.resolution_Y;
    mainBufferSpec.samples = 1;
    mainBufferSpec.colorFormats = { GL_RGBA8 };
    mainBufferSpec.colorAttachmentCount = 1;

    mainFramebuffer = std::make_unique<Framebuffer>(mainBufferSpec);


    FramebufferSpecification hdrSpec;
    hdrSpec.width = renderSettings.resolution_X;
    hdrSpec.height = renderSettings.resolution_Y;
    hdrSpec.samples = 1;
    hdrSpec.colorFormats = { GL_RGBA16F , GL_RGBA16F };
    hdrSpec.colorAttachmentCount = 2; // scene + bright

    hdrFramebuffer = std::make_unique<Framebuffer>(hdrSpec);

    FramebufferSpecification gbufferSpec;
    gbufferSpec.width = renderSettings.resolution_X;
    gbufferSpec.height = renderSettings.resolution_Y;
    gbufferSpec.samples = 1;
    gbufferSpec.colorFormats =
    {
        GL_RGBA16F,     // Position
        GL_RGBA16F,     // Normal
        GL_RGBA8,       // Albedo + Alpha
        GL_RGBA16F      // Roughness + Metallic
    };

    gbufferSpec.colorAttachmentCount = 4;
    gbufferSpec.useDepth = true;

    gbufferSpec.filtering =
    {
        GL_NEAREST,
        GL_NEAREST,
        GL_NEAREST,
        GL_NEAREST
    };

    gBuffer = std::make_unique<Framebuffer>(gbufferSpec);

    
    if (renderSettings.MSAA)
    {
        FramebufferSpecification msaaSpec;
        msaaSpec.width = renderSettings.resolution_X;
        msaaSpec.height = renderSettings.resolution_Y;
        msaaSpec.samples = renderSettings.msaaSamples;

        msaaSpec.colorFormats = { GL_RGBA16F };

        msaaSpec.colorAttachmentCount = 1;

        msaaFramebuffer = std::make_unique<Framebuffer>(msaaSpec);
    }

   
}

void RenderPipeline::PostProcess() {

}

void RenderPipeline::SetRenderSettings(const RenderSettings& settings)
{
    renderSettings = settings;

    RecreateFramebuffers();
    BuildGraph();
}


void RenderPipeline::BuildGraph()
{
    renderGraph.Clear();

    // SHADOW PASS

    renderGraph.AddPass(
        std::make_unique<ShadowPass>(
            shadowCubemap,
            shadowMap,
            assetManager
        )
    );


    if (renderSettings.renderPath == RenderPath::Forward)
    {
        // FORWARD RENDERING

        Framebuffer& target =
            renderSettings.MSAA
            ? *msaaFramebuffer
            : *hdrFramebuffer;

        // Main forward scene rendering
        renderGraph.AddPass(
            std::make_unique<ForwardPass>(
                forwardRenderer,
                target
            )
        );


        // Skybox
        renderGraph.AddPass(
            std::make_unique<SkyboxPass>(
                target,
                hdrSkybox,
                Paths::Textures + "Hdr/" + "autumn_field_puresky_4k.hdr",
                512
            )
        );

        // Resolve MSAA -> HDR framebuffer
        if (renderSettings.MSAA)
        {
            renderGraph.AddPass(
                std::make_unique<ResolvePass>(
                    *msaaFramebuffer,
                    *hdrFramebuffer
                )
            );
        }
    }
    else
    {
        // DEFERRED RENDERING

        // Geometry pass
        renderGraph.AddPass(
            std::make_unique<GeometryPass>(
                deferredRenderer,
                *gBuffer
            )
        );

        // Lighting pass
        renderGraph.AddPass(
            std::make_unique<DeferredLightingPass>(
                deferredRenderer,
                *hdrFramebuffer,
                *gBuffer
            )
        );

        renderGraph.AddPass(
            std::make_unique<DepthCopyPass>(
                *gBuffer,
                *hdrFramebuffer
            )
        );

        // Skybox after lighting
        renderGraph.AddPass(
            std::make_unique<SkyboxPass>(
                *hdrFramebuffer,
                hdrSkybox,
                Paths::Textures + "Hdr/" + "autumn_field_puresky_4k.hdr",
                512
            )
        );
    }

    // POST PROCESSING

    if (renderSettings.enableBloom)
    {
        renderGraph.AddPass(
            std::make_unique<BloomPass>(
                postProcess,
                *hdrFramebuffer,
                renderSettings.resolution_X,
                renderSettings.resolution_Y
            )
        );
    }

    // Final tonemap to backbuffer/main framebuffer
    renderGraph.AddPass(
        std::make_unique<ToneMapPass>(
            postProcess,
            *mainFramebuffer,
            *hdrFramebuffer
        )
    );
}


void RenderPipeline::Render(RenderContext& ctx)
{
    if (ctx.settings->needsReload) {

        SetRenderSettings(*ctx.settings);
        ctx.settings->needsReload = false;



    }
    // 1 Update per-frame context data
    ctx.irradianceMap = hdrSkybox.GetIrradianceMap();
    ctx.shadowMap = &shadowMap;
    ctx.shadowCubeMap = &shadowCubemap;
    ctx.prefilterMap = hdrSkybox.GetPrefilterMap();
    ctx.brdfLUTMap = hdrSkybox.GetbrdfLUTMap();
    ctx.envIntensity = hdrSkybox.envIntensity;
    ctx.envTint = hdrSkybox.envTint;
    ctx.envRotation = hdrSkybox.rot;

    // Execute passes in order
    renderGraph.Execute(ctx);
                
}

Framebuffer& RenderPipeline::GetFinalFramebuffer()
{
    return *mainFramebuffer;
}



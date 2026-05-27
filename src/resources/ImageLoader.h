#pragma once
#include <string>

#include "graphics/opengl/GLTexture.h"
#include "external/picopng.h"
#include "core/Errors.h"

#include "glm/glm.hpp"
namespace Lengine {
   

    class ImageLoader
    {
    private:

    public:

     
        static GLTexture loadPNG(const std::string& filePath);
        static GLTexture loadTexture2D(
            const std::string& filePath,
            bool srgb
        );
        static GLTexture loadTextureCubemap(std::vector<std::string> filePaths);
        static GLTexture loadHDRTextureCubemap(std::vector<std::string> faces);

        static GLTexture LoadHDRTexture(const std::string& path);
        static void ConvertEquirectangularToCubemapCPU(
            const float* hdrData,
            int hdrWidth,
            int hdrHeight,
            int faceSize,
            std::vector<float> cubemapData[6]
        );

        static std::shared_ptr<ImageData> stbiLoader(const std::string& filePath);

        static void uploadToGPU(const ImageData& img, bool srgb);
       
        
    };
}

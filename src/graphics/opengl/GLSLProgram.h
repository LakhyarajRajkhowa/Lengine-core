#pragma once

#include <string>

#include <external/GL/glew.h>
#include <external/glm/glm.hpp>
#include <external/glm/gtc/type_ptr.hpp>

#include "core/Errors.h"
namespace Lengine {


    class GLSLProgram
    {

    public:
        GLSLProgram();
        ~GLSLProgram();

        GLSLProgram(const GLSLProgram&) = delete;
        GLSLProgram& operator=(const GLSLProgram&) = delete;

        void compileShaders(const std::string& vertexShaderFilePath, const std::string& fragmentShaderFilePath);
        void compileShaders_3(
            const std::string& vertexShaderFilePath,
            const std::string& geometryShaderFilePath,
            const std::string& fragmentShaderFilePath
           );

        void linkShaders();
        void addAttribute(const std::string& attributeName);

        GLint getUniformLocation(const std::string& uniformName);

        void use();
        void unuse();
        void setMat4(const std::string& name, const glm::mat4& mat);
        void setMat3(const std::string& name, const glm::mat3& mat);
        void setVec3(const std::string& name, const glm::vec3& vec);
        void setVec4(const std::string& name, const glm::vec4& vec);
        void setFloat(const std::string& name, float value);
        void setInt(const std::string& name, int value);
        void setBool(const std::string& name, bool state);
          
    private:

        void compileShader(const std::string& filePath, GLuint id);

        int _numAttributes;
        GLuint _programID;
        GLuint _vertexShaderID;
        GLuint _fragmentShaderID;
        GLuint _geometryShaderID = 0;


    };
}




/*
    This is the Shader class. 
*/

#include <iostream>

#include <fstream>
#include <vector>

#include "../graphics/opengl/GLSLProgram.h"

namespace Lengine {

    GLSLProgram::GLSLProgram() : _numAttributes(0), _programID(0), _vertexShaderID(0), _fragmentShaderID(0)
    {
    }

    GLSLProgram::~GLSLProgram()
    {
        glDeleteProgram(_programID);
    }

    void GLSLProgram::compileShaders(const std::string& vertexShaderFilePath, const std::string& fragmentShaderFilePath)
    {

        _programID = glCreateProgram();

        _vertexShaderID = glCreateShader(GL_VERTEX_SHADER);
        if (!_vertexShaderID)
        {
            fatalError("Vertex shader failed to be created!");
        }
        _fragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);
        if (!_fragmentShaderID)
        {
            fatalError("Fragment shader failed to be created!");
        }

        compileShader(vertexShaderFilePath, _vertexShaderID);
        compileShader(fragmentShaderFilePath, _fragmentShaderID);
    }

    void GLSLProgram::compileShaders_3(
        const std::string& vertexShaderFilePath,
        const std::string& geometryShaderFilePath,
        const std::string& fragmentShaderFilePath)
    {
        _programID = glCreateProgram();

        _vertexShaderID = glCreateShader(GL_VERTEX_SHADER);
        _geometryShaderID = glCreateShader(GL_GEOMETRY_SHADER);
        _fragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

        if (!_vertexShaderID || !_geometryShaderID || !_fragmentShaderID) {
            fatalError("Shader failed to be created!");
        }

        compileShader(vertexShaderFilePath, _vertexShaderID);
        compileShader(geometryShaderFilePath, _geometryShaderID);
        compileShader(fragmentShaderFilePath, _fragmentShaderID);
    }

    void GLSLProgram::linkShaders()
    {

        glAttachShader(_programID, _vertexShaderID);
        if (_geometryShaderID != 0) {              
            glAttachShader(_programID, _geometryShaderID);
        }
        glAttachShader(_programID, _fragmentShaderID);

        glLinkProgram(_programID);

        GLint isLinked = 0;
        glGetProgramiv(_programID, GL_LINK_STATUS, (int*)&isLinked);
        if (isLinked == GL_FALSE)
        {
            GLint maxLength = 0;
            glGetProgramiv(_programID, GL_INFO_LOG_LENGTH, &maxLength);

            std::vector<GLchar> errorLog(maxLength);
            glGetProgramInfoLog(_programID, maxLength, &maxLength, &errorLog[0]);

            glDeleteProgram(_programID);
            glDeleteShader(_vertexShaderID);
            glDeleteShader(_fragmentShaderID);
            glDeleteShader(_geometryShaderID);

            printf("%s\n", &(errorLog[0]));
            fatalError("Shaders  failed to link");
        }

        glDetachShader(_programID, _vertexShaderID);
        glDetachShader(_programID, _fragmentShaderID);
        if (_geometryShaderID != 0) {
            glDetachShader(_programID, _geometryShaderID);
            glDeleteShader(_geometryShaderID);
        }
        glDeleteShader(_vertexShaderID);
        glDeleteShader(_fragmentShaderID);
    }

    void GLSLProgram::addAttribute(const std::string& attributeName)
    {

        glBindAttribLocation(_programID, _numAttributes++, attributeName.c_str());
    }

    GLint GLSLProgram::getUniformLocation(const std::string& uniformName) {
        GLuint location = glGetUniformLocation(_programID, uniformName.c_str());
        if (location == -1) {
            fatalError("Unifrom " + uniformName + " not found in shader!");
        }

        return location;

    }

    void GLSLProgram::use()
    {
        glUseProgram(_programID);
        
    }
    void GLSLProgram::unuse()
    {
        glUseProgram(0);
       
    }
    void GLSLProgram::compileShader(const std::string& filePath, GLuint id)
    {
        std::ifstream shaderFile(filePath);
        if (shaderFile.fail())
        {
            perror(filePath.c_str());
            fatalError("Error to open " + filePath);
        }

        std::string fileContents = "";
        std::string line;

        while (std::getline(shaderFile, line))
        {
            fileContents += line + "\n";
        }

        shaderFile.close();

        const char* contentsPtr = fileContents.c_str();
        glShaderSource(id, 1, &contentsPtr, nullptr);

        glCompileShader(id);

        GLint success = 0;
        glGetShaderiv(id, GL_COMPILE_STATUS, &success);

        if (success == GL_FALSE)
        {
            GLint maxLength = 0;
            glGetShaderiv(id, GL_INFO_LOG_LENGTH, &maxLength);

            std::vector<GLchar> errorLog(maxLength);
            glGetShaderInfoLog(id, maxLength, &maxLength, &errorLog[0]);

            glDeleteShader(id); 

            printf("%s\n", &(errorLog[0]));
            fatalError("Shader " + filePath + " failed to compile");


        }

    }

    void GLSLProgram::setMat4(const std::string& name, const glm::mat4& mat) {
        glUniformMatrix4fv(glGetUniformLocation(_programID, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
    }

    void GLSLProgram::setMat3(const std::string& name, const glm::mat3& mat) {
        GLint loc = glGetUniformLocation(_programID, name.c_str());
        if (loc == -1) {
            // Uniform not found
            fatalError("Warning: uniform '" + name + "' not found!\n");
            return;
        }
        glUniformMatrix3fv(loc, 1, GL_FALSE, glm::value_ptr(mat));
    }
    void GLSLProgram::setVec3(const std::string& name, const glm::vec3& vec) {
        GLint loc = glGetUniformLocation(_programID, name.c_str());
        if (loc == -1) return;  // uniform not found
        glUniform3f(loc, vec.x, vec.y, vec.z);
    }
    void GLSLProgram::setVec4(const std::string& name, const glm::vec4& vec) {
        GLint loc = glGetUniformLocation(_programID, name.c_str());
        if (loc == -1) return;  // uniform not found
        glUniform4f(loc, vec.r, vec.g, vec.b, vec.a);
    }
    void GLSLProgram::setFloat(const std::string& name, float value) {
        GLint location = glGetUniformLocation(_programID, name.c_str());
        if (location == -1) return;

        glUniform1f(location, value);
    }
    void GLSLProgram::setInt(const std::string& name, int value) {
        GLint location = glGetUniformLocation(_programID, name.c_str());
        if (location == -1) return;

        glUniform1i(location, value);
        
    }
    void GLSLProgram::setBool(const std::string& name, bool state) {
        GLint location = glGetUniformLocation(_programID, name.c_str());
        if (location == -1) return;

        glUniform1i(location, state);

    }
}




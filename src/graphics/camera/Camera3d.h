#pragma once
#include <external/glm/glm.hpp>
#include <external/glm/gtc/matrix_transform.hpp>
#include <external/glm/gtc/type_ptr.hpp>
#include <external/SDL2/SDL.h>

#include "input/InputManager.h"

namespace Lengine {
    enum class CameraControlMode {
        first = 0,
        second, 
        third
    };

    class Camera3d {
    public:
        Camera3d();
        ~Camera3d();

        void Init(
            InputManager* inputManager,
            const uint32_t width = 1280,
            const uint32_t height = 720,
            glm::vec3 cameraPos = glm::vec3(0, 5, 0),
            float FOV = 45.0f
        );
         const glm::vec3& getCameraPosition()  { return position; }
         const glm::vec3& getCameraDirection() { return front; }
         const glm::vec3& getCameraPosition() const { return position; }
         const glm::vec3& getCameraDirection() const { return front; }
         void setAspectRatio(float aspect);
         float getAspectRatio() const { return aspectRatio; }
         glm::mat4 getViewMatrix();
         const glm::mat4& getViewMatrix() const;

         void setProjectionMatrix(glm::vec4 projection);
         glm::mat4 getProjectionMatrix();
         const glm::mat4& getProjectionMatrix() const;

         glm::vec3 getRightVector();
         glm::vec3 getForwardVector();


        bool isFixed = true;

        glm::vec3 getForward() const;
        glm::vec3 getRight() const;
        glm::vec3 getUp() const;

        void move(const glm::vec3& offset);
        void rotate(float yawOffset, float pitchOffset);
        void zoom(float amount);

        void updateViewMatrix();

        CameraControlMode controlMode = CameraControlMode::first;


    private:
        glm::vec3 position = {0, 10, 0};
        glm::vec3 front;
        glm::vec3 up;
        glm::mat4 projectionMatrix;
        glm::mat4 viewMatrix;
        float yaw, pitch;
        float fov = 45.0f;
        float aspectRatio;
        float nearPlane, farPlane;

        glm::vec3 direction;
        InputManager* _inputManager;


        float speedFactor = 10.0f;
        float speedMultiplier = 1.0f;


    };

}
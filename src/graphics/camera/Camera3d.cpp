#include "Camera3d.h"

namespace Lengine {
    float _speed = 0.1f;
    Camera3d::Camera3d() {
    }
    Camera3d::~Camera3d() {}

    void Camera3d::Init(
        InputManager* inputManager,
        const uint32_t width,
        const uint32_t height,
        glm::vec3 cameraPos,
        float FOV
    ) {
        position = cameraPos;
        yaw = -90.0f;
        pitch = 0.0f;
        nearPlane = 0.50f;
        farPlane = 1000.0f;
        up = glm::vec3(0.0f, 1.0f, 0.0f);

        fov = FOV;
        aspectRatio = (static_cast<float>(width) / static_cast<float>(height));
        inputManager = inputManager;

        direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        direction.y = sin(glm::radians(pitch));
        direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(direction);
        projectionMatrix = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
    }

    void Camera3d::setAspectRatio(float aspect)
    {
        aspectRatio = aspect;   
        projectionMatrix = glm::perspective(glm::radians(fov), aspect, nearPlane, farPlane);
    }
    glm::mat4 Camera3d::getViewMatrix() {
        return viewMatrix;
    }
    const glm::mat4& Camera3d::getViewMatrix() const{
        return viewMatrix;
    }

    glm::mat4 Camera3d::getProjectionMatrix() {
        return projectionMatrix;
    }
    const glm::mat4& Camera3d::getProjectionMatrix() const {
        return projectionMatrix;
    }
    glm::vec3 Camera3d::getRightVector() {
        return glm::normalize(glm::cross(front, up));
    }

    glm::vec3 Camera3d::getForwardVector() {
        return glm::normalize(front);
    }
   


    glm::vec3 Camera3d::getForward() const {
        return glm::normalize(front);   // or -view[2]
    }

    glm::vec3 Camera3d::getRight() const {
        return glm::normalize(glm::cross(getForward(), up));
    }

    glm::vec3 Camera3d::getUp() const {
        return glm::normalize(glm::cross(getRight(), getForward()));
    }

    void Camera3d::move(const glm::vec3& offset)
    {
        position += offset;
        updateViewMatrix();
    }

    void Camera3d::rotate(float yawOffset, float pitchOffset)
    {
        yaw += yawOffset;
        pitch -= pitchOffset;

        if (pitch > 89.0f)
            pitch = 89.0f;

        if (pitch < -89.0f)
            pitch = -89.0f;

        direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        direction.y = sin(glm::radians(pitch));
        direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

        front = glm::normalize(direction);

        updateViewMatrix();
    }

    void Camera3d::zoom(float amount)
    {
        position += front * amount;
        updateViewMatrix();
    }

    void Camera3d::updateViewMatrix()
    {
        viewMatrix = glm::lookAt(position, position + front, up);
    }
}

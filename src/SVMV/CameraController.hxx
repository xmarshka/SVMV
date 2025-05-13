#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <SVMV/InputHandler.hxx>

#include <iostream>

namespace SVMV
{
    class CameraControllerNoclip : public Controller
    {
    public:
        CameraControllerNoclip() = default;
        CameraControllerNoclip(bool active, float speed, float sensitivity, glm::vec3 position, float pitch, float yaw);

        CameraControllerNoclip(const CameraControllerNoclip&) = delete;
        CameraControllerNoclip& operator=(const CameraControllerNoclip&) = delete;

        CameraControllerNoclip(CameraControllerNoclip&& other) noexcept;
        CameraControllerNoclip& operator=(CameraControllerNoclip&& other) noexcept;

        ~CameraControllerNoclip() = default;

        void Process(float deltaTime);
        void InputEvent(std::shared_ptr<Input::Event> inputEvent) override;

        void setCameraSpeed(float speed);

        glm::vec3 getCameraPosition();
        glm::vec3 getCameraFront();
        glm::vec3 getCameraUp();
        float getPitch();
        float getYaw();

    private:
        bool _active        { false };
        float _speed        { 0.0f };
        float _sensitivity  { 0.0f };

        glm::vec3 _position     { 0.0f, 0.0f, 0.0f };
        glm::vec3 _front        { 0.0f, 0.0f, -1.0f };
        glm::vec3 _up           { 0.0f, 1.0f, 0.0f };
        float _pitch            { 0.0f };
        float _yaw              { 0.0f };

        float _desiredPitch     { 0.0f };
        float _desiredYaw       { 0.0f };

        glm::vec3 _toMove       { 0.0f, 0.0f, 0.0f };
        float _toPitch          { 0.0f };
        float _toYaw            { 0.0f };
    };
}
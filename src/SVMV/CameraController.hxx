#pragma once

#include <glm/glm.hpp>

#include <SVMV/InputHandler.hxx>

#include <iostream>

namespace SVMV
{
    class CameraControllerNoclip : public Controller
    {
    public:
        CameraControllerNoclip() = default;
        CameraControllerNoclip(bool active, glm::vec3 position, float pitch, float yaw);

        CameraControllerNoclip(const CameraControllerNoclip&) = delete;
        CameraControllerNoclip& operator=(const CameraControllerNoclip&) = delete;

        CameraControllerNoclip(CameraControllerNoclip&& other) noexcept;
        CameraControllerNoclip& operator=(CameraControllerNoclip&& other) noexcept;

        ~CameraControllerNoclip() = default;

        void InputEvent(std::shared_ptr<Input::Event> inputEvent) override;

    private:
        bool _active        { false };

        glm::vec3 _position     { 0, 0, 0 };
        float _pitch            { 0.0f };
        float _yaw              { 0.0f };
    };
}
#pragma once

#include <SVMV/GLFWwindowWrapper.hxx>
#include <SVMV/Loader.hxx>
#include <SVMV/VulkanRenderer.hxx>
#include <SVMV/InputHandler.hxx>
#include <SVMV/CameraController.hxx>

#include <chrono>

namespace SVMV
{
    class Application
    {
    private:
        GLFWwindowWrapper _window{ 1280, 1280, "SVMV", this };
        VulkanRenderer _renderer{ 1280, 1280, "SVMV", 2, _window };

        InputHandler _inputHandler;
        CameraControllerNoclip _cameraController{ true, 4.0f, 3.0f, glm::vec3(0.0f, 0.0f, 2.0f), 0.0f, -90.0f };

    public:
        Application() = delete;
        Application(int width, int height, const std::string& name);

        Application(const Application&) = delete;
        Application& operator=(const Application&) = delete;

        Application(Application&& other) = delete;
        Application& operator=(Application&& other) = delete;

        ~Application() = default;

    private:
        void loop();

        static void resizedCallback(GLFWwindow* window, int width, int height);
        static void minimizedCallback(GLFWwindow* window, int minimized);
        static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
        static void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos);
    };
}

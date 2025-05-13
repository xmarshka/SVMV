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
        GLFWwindowWrapper _window{ 1440, 1440, "SVMV", this };
        VulkanRenderer _renderer{ 1440, 1440, "SVMV", 3, _window };

        InputHandler _inputHandler;
        CameraControllerNoclip _cameraController{ true, 0.3f, 3.0f, glm::vec3(1.22f, 0.0f, 2.14f), -0.16f, -115.0f }; // true, 0.3f, 3.06f, glm::vec3(1.22f, 0.0f, 2.14f), -0.16f, -115.0f

        bool _inMenu{ false };
        bool _minimized{ false };

    public:
        Application() = delete;
        Application(int width, int height, const std::string& name, const std::string& fileToLoad);

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
        static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    };
}

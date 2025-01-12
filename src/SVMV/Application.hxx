#pragma once

#include <SVMV/GLFWwindowWrapper.hxx>
#include <SVMV/Loader.hxx>
#include <SVMV/VulkanRenderer.hxx>
#include <SVMV/InputHandler.hxx>
#include <SVMV/CameraController.hxx>

namespace SVMV
{
    class Application
    {
    private:
        GLFWwindowWrapper _window{ 800, 600, "SVMV", this };
        VulkanRenderer _renderer{ 800, 600, "SVMV", 2, _window }; // TODO: placeholder, implement move constructor later (or initialize function)

        InputHandler _inputHandler;
        CameraControllerNoclip _cameraController{ true, glm::vec3(0.0f, 0.0f, 0.0f), 0.0f, 0.0f };

        // TODO:
        //      move GLFWWindowWrapper here,
        //      pass GLFWwindow* to VulkanRenderer so it can create its surface using it
        //      set the user pointer to be this (Application) and call the resized and minimized functions from the renderer
        //          the point of this is to allow the input handler to use the GLFW interupts

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
    };
}

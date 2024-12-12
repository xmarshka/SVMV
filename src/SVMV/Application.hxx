#pragma once

#include <SVMV/Loader.hxx>
#include <SVMV/VulkanRenderer.hxx>

namespace SVMV
{
    class Application
    {
    private:
        VulkanRenderer _renderer{ 800, 600, "SVMV", 2 }; // TODO: placeholder, implement move constructor later (or initialize function)

        bool _frozen{ false };

    public:
        Application() = delete;
        Application(int width, int height, const std::string& name);

        Application(const Application&) = delete;
        Application& operator=(const Application&) = delete;

        Application(Application&& other) = delete;
        Application& operator=(Application&& other) = delete;

        ~Application();

    private:
        void initialize(int width, int height, const std::string& name);
        void cleanup();
        void loop();

        static void framebufferResized(GLFWwindow* window, int width, int height);
        static void minimized(GLFWwindow* window, int minimized);
    };
}

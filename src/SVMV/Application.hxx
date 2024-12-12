#pragma once

#include <SVMV/Loader.hxx>
#include <SVMV/VulkanRenderer.hxx>

namespace SVMV
{
    class Application
    {
    private:
        VulkanRenderer _renderer;

        bool _frozen{ false };

    public:
        Application() = delete;
        Application(const Application&) = delete;
        Application(int width, int height, const std::string& name);
        ~Application();

    private:
        void initialize(int width, int height, const std::string& name);
        void cleanup();
        void loop();

        static void framebufferResized(GLFWwindow* window, int width, int height);
        static void minimized(GLFWwindow* window, int minimized);
    };
}

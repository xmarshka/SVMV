#pragma once

#include <SVMV/Loader.hxx>
#include <SVMV/VulkanRenderer.hxx>

namespace SVMV
{
    class Application
    {
    private:
        GLFWwindow* _window;
        VulkanRenderer _renderer;

        bool _frozen;

    public:
        Application() = delete;
        Application(const Application&) = delete;
        Application(int width, int height, const std::string& name);
        ~Application();

    private:
        void initialize(int width, int height, const std::string& name);
        void cleanup();
        void loop();

        VkSurfaceKHR createGLFWWindowAndSurface(int width, int height, const std::string& name);

        static void framebufferResized(GLFWwindow* window, int width, int height);
        static void minimized(GLFWwindow* window, int minimized);
    };
}

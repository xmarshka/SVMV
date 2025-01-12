#pragma once

#include <vulkan/vulkan_raii.hpp>
#include <GLFW/glfw3.h>

namespace SVMV
{
    class GLFWwindowWrapper
    {
    public:
        GLFWwindowWrapper() = default;
        GLFWwindowWrapper(int width, int height, const std::string& name, void* userPointer);

        GLFWwindowWrapper(const GLFWwindowWrapper&) = delete;
        GLFWwindowWrapper& operator=(const GLFWwindowWrapper&) = delete;

        GLFWwindowWrapper(GLFWwindowWrapper&& other) noexcept;
        GLFWwindowWrapper& operator=(GLFWwindowWrapper&& other) noexcept;

        ~GLFWwindowWrapper();

        GLFWwindow* getWindow() const noexcept;

        vk::raii::SurfaceKHR createVulkanSurface(const vk::raii::Instance& instance) const;
    private:
        GLFWwindow* _window{ nullptr };
    };
}

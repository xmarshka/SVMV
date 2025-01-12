#include <SVMV/GLFWwindowWrapper.hxx>

using namespace SVMV;

GLFWwindowWrapper::GLFWwindowWrapper(int width, int height, const std::string& name, void* userPointer)
{
    if (glfwInit() != GLFW_TRUE)
    {
        throw std::runtime_error("glfw: failed to initialize glfw");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    _window = glfwCreateWindow(width, height, name.c_str(), nullptr, nullptr);
    if (_window == nullptr)
    {
        throw std::runtime_error("glfw: failed to create window");
    }

    glfwSetWindowUserPointer(_window, userPointer);

    //glfwSetFramebufferSizeCallback(_window, resizeCallback);
    //glfwSetWindowIconifyCallback(_window, minimizedCallback);
}

GLFWwindowWrapper::GLFWwindowWrapper(GLFWwindowWrapper&& other) noexcept
{
    this->_window = other._window;

    other._window = nullptr;
}

GLFWwindowWrapper& GLFWwindowWrapper::operator=(GLFWwindowWrapper&& other) noexcept
{
    if (this != &other)
    {
        if (_window != nullptr)
        {
            glfwDestroyWindow(_window);
        }

        this->_window = other._window;

        other._window = nullptr;
    }

    return *this;
}

GLFWwindowWrapper::~GLFWwindowWrapper()
{
    glfwDestroyWindow(_window);
}

GLFWwindow* GLFWwindowWrapper::getWindow() const noexcept
{
    return _window;
}

vk::raii::SurfaceKHR GLFWwindowWrapper::createVulkanSurface(const vk::raii::Instance& instance) const
{
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(*instance, _window, nullptr, &surface) != VK_SUCCESS)
    {
        throw std::runtime_error("glfw: failed to create window surface");
    }

    return vk::raii::SurfaceKHR(instance, surface);
}

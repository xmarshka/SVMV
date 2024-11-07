#include <SVMV/Application.hxx>

using namespace SVMV;

Application::Application(int width, int height, const std::string& name) : _frozen(false)
{
    initialize(width, height, name);

    loop();
}

Application::~Application()
{
    cleanup();
}

void Application::initialize(int width, int height, const std::string& name)
{
    if (glfwInit() == GLFW_FALSE)
    {
        throw std::runtime_error("glfw: failed to initialize glfw");
    }

    _renderer.createInstance();
    _renderer.initializeRenderer(vk::SurfaceKHR(createGLFWWindowAndSurface(width, height, name)));

    _renderer.loadScene(Loader::loadScene(RESOURCE_DIR"/models/SimpleMeshes.gltf"));
}

void Application::cleanup()
{
    _renderer.cleanup();

    glfwDestroyWindow(_window);

    glfwTerminate();
}

void Application::loop()
{
    while (!glfwWindowShouldClose(_window))
    {
        if (_frozen)
        {
            glfwPollEvents();
        }
        else
        {
            glfwPollEvents();
            _renderer.draw();
        }
    }

    _renderer.getDevice().waitIdle();
}

VkSurfaceKHR Application::createGLFWWindowAndSurface(int width, int height, const std::string& name)
{
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    //glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    _window = glfwCreateWindow(width, height, name.c_str(), nullptr, nullptr);

    glfwSetWindowUserPointer(_window, this);
    glfwSetFramebufferSizeCallback(_window, framebufferResized);
    glfwSetWindowIconifyCallback(_window, minimized);

    VkSurfaceKHR surface;

    if (glfwCreateWindowSurface(_renderer.getInstance(), _window, nullptr, &surface) != VK_SUCCESS)
    {
        throw std::runtime_error("glfw: failed to create window surface");
    }

    return surface;
}

void Application::framebufferResized(GLFWwindow* window, int width, int height)
{
    SVMV::Application* application = reinterpret_cast<SVMV::Application*>(glfwGetWindowUserPointer(window));
    application->_renderer.resized = true;
}

void Application::minimized(GLFWwindow* window, int minimized)
{
    SVMV::Application* application = reinterpret_cast<SVMV::Application*>(glfwGetWindowUserPointer(window));

    if (minimized)
    {
        application->_frozen = true;
    }
    else
    {
        application->_frozen = false;
    }
}

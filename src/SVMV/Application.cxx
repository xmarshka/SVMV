#include <SVMV/Application.hxx>

using namespace SVMV;

Application::Application(int width, int height, const std::string& name)
{
    initialize(width, height, name);

    loop();
}

void Application::initialize(int width, int height, const std::string& name)
{
    _renderer.loadScene(Loader::loadScene(RESOURCE_DIR"/models/Box.gltf"));
}

void Application::loop()
{
    while (!glfwWindowShouldClose(_renderer.getWindow()))
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

void Application::framebufferResized(GLFWwindow* window, int width, int height)
{
    SVMV::Application* application = reinterpret_cast<SVMV::Application*>(glfwGetWindowUserPointer(window));
    //application->_renderer.resized = true;
}

void Application::minimized(GLFWwindow* window, int minimized)
{
    SVMV::Application* application = reinterpret_cast<SVMV::Application*>(glfwGetWindowUserPointer(window));

    //if (minimized)
    //{
    //    application->_frozen = true;
    //}
    //else
    //{
    //    application->_frozen = false;
    //}
}

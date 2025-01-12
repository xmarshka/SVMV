#include <SVMV/Application.hxx>

using namespace SVMV;

Application::Application(int width, int height, const std::string& name)
{
    glfwSetFramebufferSizeCallback(_window.getWindow(), resizedCallback);
    glfwSetWindowIconifyCallback(_window.getWindow(), minimizedCallback);

    glfwSetKeyCallback(_window.getWindow(), keyCallback);

    _inputHandler.registerController(&_cameraController);

    _renderer.loadScene(Loader::loadScene(RESOURCE_DIR"/models/BoxTextured.gltf"));

    loop();
}

void Application::loop()
{
    while (!glfwWindowShouldClose(_window.getWindow()))
    {
        glfwPollEvents();
        _inputHandler.signalEvents();
        _renderer.draw();
    }

    _renderer.getDevice().waitIdle();
}

void Application::resizedCallback(GLFWwindow* window, int width, int height)
{
    Application* application = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
    application->_renderer.resize(width, height);
}

void Application::minimizedCallback(GLFWwindow* window, int minimized)
{
    Application* application = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));

    if (minimized)
    {
        application->_renderer.minimize();
    }
    else
    {
        application->_renderer.maximize();
    }
}

void Application::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    Application* application = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
    application->_inputHandler.glfwKeyCallback(key, scancode, action, mods);
}

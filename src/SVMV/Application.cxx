#include <SVMV/Application.hxx>

using namespace SVMV;

Application::Application(int width, int height, const std::string& name)
{
    //glfwSetFramebufferSizeCallback(_window.getWindow(), resizedCallback);
    glfwSetWindowIconifyCallback(_window.getWindow(), minimizedCallback);
    glfwSetKeyCallback(_window.getWindow(), keyCallback);
    glfwSetCursorPosCallback(_window.getWindow(), cursorPositionCallback);
    glfwSetScrollCallback(_window.getWindow(), scrollCallback);

    glfwSetInputMode(_window.getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    _inputHandler.registerController(&_cameraController);
    _inputHandler.ignoreFirstMouseMovement();

    _renderer.loadScene(Loader::loadScene(RESOURCE_DIR"/models/Corset.glb"));

    loop();
}

void Application::loop()
{
    while (!glfwWindowShouldClose(_window.getWindow()))
    {
        glfwPollEvents();

        if (_minimized)
        {
            return;
        }

        std::chrono::high_resolution_clock::time_point time1 = std::chrono::high_resolution_clock::now();

        _inputHandler.signalEvents();
        _renderer.draw();
        _renderer.setCamera(_cameraController.getCameraPosition(), _cameraController.getCameraFront(), _cameraController.getCameraUp(), 75.0f);

        std::chrono::high_resolution_clock::time_point time2 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> deltaTime = std::chrono::duration_cast<std::chrono::duration<float>>(time2 - time1);

        _cameraController.Process(deltaTime.count());
    }

    _renderer.getDevice().waitIdle();
}

void Application::minimizedCallback(GLFWwindow* window, int minimized)
{
    // code taken from: https://vulkan-tutorial.com/Drawing_a_triangle/Swap_chain_recreation

    int width = 0;
    int height = 0;

    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0)
    {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }
}

void Application::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    Application* application = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));

    if (key == GLFW_KEY_TAB && action == GLFW_PRESS)
    {
        if (application->_inMenu)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
        else
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            application->_inputHandler.clearHeldKeys();
        }

        application->_inMenu = !application->_inMenu;
        application->_inputHandler.ignoreFirstMouseMovement();
    }

    if (!application->_inMenu)
    {
        application->_inputHandler.glfwKeyCallback(key, scancode, action, mods);
    }
    else
    {
        ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
    }
}

void Application::cursorPositionCallback(GLFWwindow* window, double xpos, double ypos)
{
    Application* application = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));

    if (!application->_inMenu)
    {
        application->_inputHandler.glfwCursorPositionCallback(xpos, ypos);
    }
    else
    {
        ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);
    }
}

void Application::scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    Application* application = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));

    if (!application->_inMenu)
    {
        application->_inputHandler.glfwScrollCallback(xoffset, yoffset);
    }
    else
    {
        ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
    }
}

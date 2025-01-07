#include <SVMV/Application.hxx>

using namespace SVMV;

Application::Application(int width, int height, const std::string& name)
{
    _renderer.loadScene(Loader::loadScene(RESOURCE_DIR"/models/BoxTextured.gltf"));

    loop();
}

void Application::loop()
{
    while (!glfwWindowShouldClose(_renderer.getWindow()))
    {
        glfwPollEvents();
        _renderer.draw();
    }

    _renderer.getDevice().waitIdle();
}

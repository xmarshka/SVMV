#include <SVMV/Application.hxx>

int main(int argc, char** argv)
{
    if (argc > 1)
    {
        SVMV::Application application(800, 600, "SVMV", argv[1]);
    }
    else
    {
        SVMV::Application application(800, 600, "SVMV", "");
    }

    glfwTerminate(); // TODO: move this somewhere out of main

    return 0;
}

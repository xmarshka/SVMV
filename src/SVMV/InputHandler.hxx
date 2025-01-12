#pragma once

#include <SVMV/Input.hxx>

#include <GLFW/glfw3.h>

#include <queue>
#include <memory>
#include <vector>
#include <unordered_map>

namespace SVMV
{
    class Controller
    {
    public:
        virtual ~Controller() = default;

        virtual void InputEvent(std::shared_ptr<Input::Event> inputEvent) {}
    };

    class InputHandler
    {
    public:
        InputHandler();

        InputHandler(const InputHandler&) = delete;
        InputHandler& operator=(const InputHandler&) = delete;

        InputHandler(InputHandler&& other) noexcept;
        InputHandler& operator=(InputHandler&& other) noexcept;

        ~InputHandler() = default;

        void signalEvents();

        void registerController(Controller* controller);

        void glfwKeyCallback(int key, int scancode, int action, int mods);

    private:
        std::queue<std::shared_ptr<Input::Event>> _eventQueue;

        std::vector<Controller*> _controllers;

        std::unordered_map<int, bool> _keyHeldMap;
        Input::MouseDelta _mouseDelta;
    };
}
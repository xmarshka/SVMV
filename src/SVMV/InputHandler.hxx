#pragma once

#include <SVMV/Input.hxx>

#include <GLFW/glfw3.h>

#include <queue>
#include <memory>
#include <vector>
#include <unordered_map>

#include <iostream>

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

        void clearHeldKeys();
        void ignoreFirstMouseMovement();

        void glfwKeyCallback(int key, int scancode, int action, int mods);
        void glfwCursorPositionCallback(double xpos, double ypos);
        void glfwScrollCallback(double xoffset, double yoffset);

    private:
        std::queue<std::shared_ptr<Input::Event>> _eventQueue;

        std::vector<Controller*> _controllers;

        std::unordered_map<int, bool> _keyHeldMap;

        Input::MouseDelta _previousMousePosition{ 0.0f, 0.0f };
        Input::MouseDelta _mouseDelta{ 0.0f, 0.0f };
        bool _ignoreFirstMouseMovement{ false };
    };
}
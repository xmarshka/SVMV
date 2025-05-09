#include <SVMV/InputHandler.hxx>

using namespace SVMV;

InputHandler::InputHandler()
{
    _keyHeldMap[static_cast<int>(Input::KeyCode::W)] = false;
    _keyHeldMap[static_cast<int>(Input::KeyCode::A)] = false;
    _keyHeldMap[static_cast<int>(Input::KeyCode::S)] = false;
    _keyHeldMap[static_cast<int>(Input::KeyCode::D)] = false;
    _keyHeldMap[static_cast<int>(Input::KeyCode::LEFT_SHIFT)] = false;
    _keyHeldMap[static_cast<int>(Input::KeyCode::LEFT_CONTROL)] = false;
}

InputHandler::InputHandler(InputHandler&& other) noexcept
{
    this->_controllers = std::move(other._controllers);
    this->_keyHeldMap = std::move(other._keyHeldMap);
    this->_mouseDelta = other._mouseDelta;

    other._mouseDelta = Input::MouseDelta(0.0f, 0.0f);
}

InputHandler& InputHandler::operator=(InputHandler&& other) noexcept
{
    if (this != &other)
    {
        this->_controllers = std::move(other._controllers);
        this->_keyHeldMap = std::move(other._keyHeldMap);
        this->_mouseDelta = other._mouseDelta;

        other._mouseDelta = Input::MouseDelta(0.0f, 0.0f);
    }

    return *this;
}

void InputHandler::signalEvents()
{
    while (!_eventQueue.empty())
    {
        for (const auto& controller : _controllers)
        {
            controller->InputEvent(_eventQueue.front());
        }

        _eventQueue.pop();
    }

    for (const auto& controller : _controllers)
    {
        for (auto& key : _keyHeldMap)
        {
            if (key.second) // key is held
            {
                controller->InputEvent(std::make_shared<Input::KeyEvent>(static_cast<Input::KeyCode>(key.first), Input::KeyState::HELD));
            }
        }
    }
}

void InputHandler::registerController(Controller* controller)
{
    _controllers.push_back(controller);
}

void InputHandler::clearHeldKeys()
{
    for (auto& keyHeld : _keyHeldMap)
    {
        keyHeld.second = false;
    }
}

void InputHandler::ignoreFirstMouseMovement()
{
    _ignoreFirstMouseMovement = true;
}

void InputHandler::glfwKeyCallback(int key, int scancode, int action, int mods)
{
    switch (key)
    {
    case GLFW_KEY_W:
        switch (action)
        {
        case GLFW_PRESS:
            _eventQueue.push(std::make_shared<Input::KeyEvent>(Input::KeyCode::W, Input::KeyState::PRESSED));
            _keyHeldMap[static_cast<int>(Input::KeyCode::W)] = true;
            break;
        case GLFW_RELEASE:
            _eventQueue.push(std::make_shared<Input::KeyEvent>(Input::KeyCode::W, Input::KeyState::RELEASED));
            _keyHeldMap[static_cast<int>(Input::KeyCode::W)] = false;
            break;
        default:
            break;
        }
        break;
    case GLFW_KEY_A:
        switch (action)
        {
        case GLFW_PRESS:
            _eventQueue.push(std::make_shared<Input::KeyEvent>(Input::KeyCode::A, Input::KeyState::PRESSED));
            _keyHeldMap[static_cast<int>(Input::KeyCode::A)] = true;
            break;
        case GLFW_RELEASE:
            _eventQueue.push(std::make_shared<Input::KeyEvent>(Input::KeyCode::A, Input::KeyState::RELEASED));
            _keyHeldMap[static_cast<int>(Input::KeyCode::A)] = false;
            break;
        default:
            break;
        }
        break;
    case GLFW_KEY_S:
        switch (action)
        {
        case GLFW_PRESS:
            _eventQueue.push(std::make_shared<Input::KeyEvent>(Input::KeyCode::S, Input::KeyState::PRESSED));
            _keyHeldMap[static_cast<int>(Input::KeyCode::S)] = true;
            break;
        case GLFW_RELEASE:
            _eventQueue.push(std::make_shared<Input::KeyEvent>(Input::KeyCode::S, Input::KeyState::RELEASED));
            _keyHeldMap[static_cast<int>(Input::KeyCode::S)] = false;
            break;
        default:
            break;
        }
        break;
    case GLFW_KEY_D:
        switch (action)
        {
        case GLFW_PRESS:
            _eventQueue.push(std::make_shared<Input::KeyEvent>(Input::KeyCode::D, Input::KeyState::PRESSED));
            _keyHeldMap[static_cast<int>(Input::KeyCode::D)] = true;
            break;
        case GLFW_RELEASE:
            _eventQueue.push(std::make_shared<Input::KeyEvent>(Input::KeyCode::D, Input::KeyState::RELEASED));
            _keyHeldMap[static_cast<int>(Input::KeyCode::D)] = false;
            break;
        default:
            break;
        }
        break;
    case GLFW_KEY_LEFT_SHIFT:
        switch (action)
        {
        case GLFW_PRESS:
            _eventQueue.push(std::make_shared<Input::KeyEvent>(Input::KeyCode::LEFT_SHIFT, Input::KeyState::PRESSED));
            _keyHeldMap[static_cast<int>(Input::KeyCode::LEFT_SHIFT)] = true;
            break;
        case GLFW_RELEASE:
            _eventQueue.push(std::make_shared<Input::KeyEvent>(Input::KeyCode::LEFT_SHIFT, Input::KeyState::RELEASED));
            _keyHeldMap[static_cast<int>(Input::KeyCode::LEFT_SHIFT)] = false;
            break;
        default:
            break;
        }
        break;
    case GLFW_KEY_LEFT_CONTROL:
        switch (action)
        {
        case GLFW_PRESS:
            _eventQueue.push(std::make_shared<Input::KeyEvent>(Input::KeyCode::LEFT_CONTROL, Input::KeyState::PRESSED));
            _keyHeldMap[static_cast<int>(Input::KeyCode::LEFT_CONTROL)] = true;
            break;
        case GLFW_RELEASE:
            _eventQueue.push(std::make_shared<Input::KeyEvent>(Input::KeyCode::LEFT_CONTROL, Input::KeyState::RELEASED));
            _keyHeldMap[static_cast<int>(Input::KeyCode::LEFT_CONTROL)] = false;
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
}

void InputHandler::glfwCursorPositionCallback(double xpos, double ypos)
{
    if (_ignoreFirstMouseMovement)
    {
        _ignoreFirstMouseMovement = false;

        _previousMousePosition.x = xpos;
        _previousMousePosition.y = ypos;

        return;
    }

    Input::MouseDelta mouseDelta(xpos - _previousMousePosition.x, ypos - _previousMousePosition.y);

    _eventQueue.push(std::make_shared<Input::MouseMovementEvent>(mouseDelta));

    _previousMousePosition.x = xpos;
    _previousMousePosition.y = ypos;
}

void InputHandler::glfwScrollCallback(double xoffset, double yoffset)
{
    if (yoffset > 0.0)
    {
        _eventQueue.push(std::make_shared<Input::KeyEvent>(Input::KeyCode::SCROLL_UP, Input::KeyState::PRESSED));
    }
    else
    {
        _eventQueue.push(std::make_shared<Input::KeyEvent>(Input::KeyCode::SCROLL_DOWN, Input::KeyState::PRESSED));
    }
}

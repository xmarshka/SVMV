#include <SVMV/InputHandler.hxx>

using namespace SVMV;

InputHandler::InputHandler()
{
    _keyHeldMap[static_cast<int>(Input::KeyCode::W)] = false;
    _keyHeldMap[static_cast<int>(Input::KeyCode::A)] = false;
    _keyHeldMap[static_cast<int>(Input::KeyCode::S)] = false;
    _keyHeldMap[static_cast<int>(Input::KeyCode::D)] = false;
}

InputHandler::InputHandler(InputHandler&& other) noexcept
{
    this->_controllers = std::move(other._controllers);
    this->_keyHeldMap = std::move(other._keyHeldMap);
    this->_mouseDelta = other._mouseDelta;

    other._mouseDelta = Input::MouseDelta();
}

InputHandler& InputHandler::operator=(InputHandler&& other) noexcept
{
    if (this != &other)
    {
        this->_controllers = std::move(other._controllers);
        this->_keyHeldMap = std::move(other._keyHeldMap);
        this->_mouseDelta = other._mouseDelta;

        other._mouseDelta = Input::MouseDelta();
    }

    return *this;
}

void InputHandler::signalEvents()
{
    while (!_eventQueue.empty())
    {
        for (const auto& controller : _controllers)
        {
            controller->InputEvent(_eventQueue.front()->type, _eventQueue.front());
        }

        _eventQueue.pop();
    }

    for (const auto& controller : _controllers)
    {
        for (auto& key : _keyHeldMap)
        {
            if (key.second) // key is held
            {
                controller->InputEvent(Input::EventType::KEY, std::make_shared<Input::KeyEvent>(static_cast<Input::KeyCode>(key.first), Input::KeyState::HELD));
            }
        }
    }
}

void InputHandler::registerController(Controller* controller)
{
    _controllers.push_back(controller);
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
    default:
        break;
    }
}

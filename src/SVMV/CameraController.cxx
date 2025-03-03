#include <SVMV/CameraController.hxx>

using namespace SVMV;

CameraControllerNoclip::CameraControllerNoclip(bool active, float speed, float sensitivity, glm::vec3 position, float pitch, float yaw)
    : _active(active), _speed(speed), _sensitivity(sensitivity), _position(position), _pitch(pitch), _yaw(yaw)
{}

CameraControllerNoclip::CameraControllerNoclip(CameraControllerNoclip&& other) noexcept
{
    this->_active = other._active;
    this->_position = other._position;
    this->_pitch = other._pitch;
    this->_yaw = other._yaw;

    other._active = false;
    other._position = glm::vec3(0.0f, 0.0f, 0.0f);
    other._pitch = 0.0f;
    other._yaw = 0.0f;
}

CameraControllerNoclip& CameraControllerNoclip::operator=(CameraControllerNoclip&& other) noexcept
{
    if (this != &other)
    {
        this->_active = other._active;
        this->_position = other._position;
        this->_pitch = other._pitch;
        this->_yaw = other._yaw;

        other._active = false;
        other._position = glm::vec3(0.0f, 0.0f, 0.0f);
        other._pitch = 0.0f;
        other._yaw = 0.0f;
    }

    return *this;
}

void CameraControllerNoclip::Process(float deltaTime)
{
    if (_active)
    {
        if (_toMove != glm::vec3(0.0f, 0.0f, 0.0f))
        {
            _position += glm::normalize(_toMove) * _speed * deltaTime;
        }

        _pitch += _toPitch * _sensitivity * deltaTime;
        _yaw += _toYaw * _sensitivity * deltaTime;

        _pitch = std::min(std::max(_pitch, -85.0f), 85.0f);

        /*_front.x = std::cos(glm::radians(_yaw)) * std::cos(glm::radians(_pitch));
        _front.y = std::sin(glm::radians(_pitch));
        _front.z = std::sin(glm::radians(_yaw)) * std::cos(glm::radians(_pitch));

        _front = glm::normalize(_front);*/

        _front = glm::normalize(glm::vec3(0.0f, 0.0f, 0.0f) - _position);

        _toMove = glm::vec3(0.0f, 0.0f, 0.0f);
        _toPitch = 0.0f;
        _toYaw = 0.0f;
    }
}

void CameraControllerNoclip::InputEvent(std::shared_ptr<Input::Event> inputEvent)
{
    if (_active)
    {
        if (inputEvent->getType() == Input::EventType::KEY)
        {
            Input::KeyEvent* keyEvent = reinterpret_cast<Input::KeyEvent*>(inputEvent.get());

            if (keyEvent->keyCode == Input::KeyCode::W)
            {
                if (keyEvent->keyState == Input::KeyState::HELD)
                {
                    _toMove += _front;
                }
            }
            if (keyEvent->keyCode == Input::KeyCode::A)
            {
                if (keyEvent->keyState == Input::KeyState::HELD)
                {
                    _toMove -= glm::normalize(glm::cross(_front, _up));
                }
            }
            if (keyEvent->keyCode == Input::KeyCode::S)
            {
                if (keyEvent->keyState == Input::KeyState::HELD)
                {
                    _toMove -= _front;
                }
            }
            if (keyEvent->keyCode == Input::KeyCode::D)
            {
                if (keyEvent->keyState == Input::KeyState::HELD)
                {
                    _toMove += glm::normalize(glm::cross(_front, _up));
                }
            }
            if (keyEvent->keyCode == Input::KeyCode::LEFT_SHIFT)
            {
                if (keyEvent->keyState == Input::KeyState::HELD)
                {
                    _toMove += _up;
                }
            }
            if (keyEvent->keyCode == Input::KeyCode::LEFT_CONTROL)
            {
                if (keyEvent->keyState == Input::KeyState::HELD)
                {
                    _toMove -= _up;
                }
            }
        }
        else if (inputEvent->getType() == Input::EventType::MOUSE_MOVEMENT)
        {
            Input::MouseMovementEvent* mouseMovementEvent = reinterpret_cast<Input::MouseMovementEvent*>(inputEvent.get());

            _toYaw += mouseMovementEvent->mouseDelta.x;
            _toPitch += mouseMovementEvent->mouseDelta.y * -1.0f;
        }
    }
}

void CameraControllerNoclip::setCameraSpeed(float speed)
{
    _speed = speed;
}

glm::vec3 CameraControllerNoclip::getCameraPosition()
{
    return _position;
}

glm::vec3 CameraControllerNoclip::getCameraFront()
{
    return _front;
}

glm::vec3 CameraControllerNoclip::getCameraUp()
{
    return _up;
}

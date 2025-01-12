#include <SVMV/CameraController.hxx>

using namespace SVMV;

CameraControllerNoclip::CameraControllerNoclip(bool active, glm::vec3 position, float pitch, float yaw)
    : _active(active), _position(position), _pitch(pitch), _yaw(yaw)
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

void CameraControllerNoclip::InputEvent(Input::EventType type, std::shared_ptr<Input::Event> inputEvent)
{
    if (type == Input::EventType::KEY)
    {
        if (reinterpret_cast<Input::KeyEvent*>(inputEvent.get())->keyCode == Input::KeyCode::W)
        {
            std::cout << "W\n";
        }
    }
}

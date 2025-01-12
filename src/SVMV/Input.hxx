#pragma once

namespace SVMV
{
    namespace Input
    {
        enum class EventType : int
        {
            UNDEFINED, MOUSE_MOVEMENT, KEY
        };

        enum class KeyCode : int
        {
            W, A, S, D
        };

        enum class KeyState : int
        {
            UNDEFINED, PRESSED, HELD, RELEASED
        };

        struct MouseDelta
        {
            float x{ 0.0f };
            float y{ 0.0f };
        };

        struct Event
        {
            virtual EventType getType() { return EventType::UNDEFINED; }
            virtual ~Event() = default;
        };

        struct MouseMovementEvent : public Event
        {
            MouseMovementEvent(MouseDelta mouseDelta) : mouseDelta(mouseDelta) {}
            EventType getType() override { return EventType::MOUSE_MOVEMENT; }

            MouseDelta mouseDelta;
        };

        struct KeyEvent : public Event
        {
            KeyEvent(KeyCode keyCode, KeyState keyState) : keyCode(keyCode), keyState(keyState) {}
            EventType getType() override { return EventType::KEY; }

            KeyCode keyCode;
            KeyState keyState;
        };
    }
}
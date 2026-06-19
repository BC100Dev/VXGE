#ifndef VXGE_EVENT_HPP
#define VXGE_EVENT_HPP

#include <cstdint>
#include "VXKey.hpp"

namespace VX {
    enum class VXEventType : uint32_t {
        None = 0,

        Quit = 0x01000001,

        KeyDown = 0x02000001,
        KeyUp = 0x02000002,

        MouseMove = 0x03000001,
        MouseDown = 0x03000002,
        MouseUp = 0x03000003,
        MouseWheel = 0x03000004,

        AudioDeviceAdded = 0x04000001,
        AudioDeviceRemoved = 0x04000002,

        GamepadAdded = 0x05000001,
        GamepadRemoved = 0x05000002,
        GamepadAxis = 0x05000003,
        GamepadButton = 0x05000004,

        WindowResized = 0x06000001,
        WindowFocusGained = 0x06000002,
        WindowFocusLost = 0x06000003,
        WindowMinimized = 0x06000004,
        WindowRestored = 0x06000005,
    };

    struct VXKeyEvent {
        VXKey keyCode;
        bool repeat;
    };

    struct VXMouseEvent {
        float x;
        float y;
        float deltaX;
        float deltaY;
        float wheelX;
        float wheelY;
        uint8_t button;
    };

    struct VXAudioDeviceEvent {
        uint32_t deviceId;
        bool isCapture;
    };

    struct VXGamepadEvent {
        int32_t gamepadId;
        uint8_t button;
        int16_t axisValue;
        uint8_t axis;
    };

    struct VXWindowEvent {
        uint32_t width;
        uint32_t height;
    };

    struct VXEvent {
        VXEventType type = VXEventType::None;

        union {
            VXKeyEvent key;
            VXMouseEvent mouse;
            VXAudioDeviceEvent audio;
            VXGamepadEvent gamepad;
            VXWindowEvent window;
        };

        SDL_Event ToSDL() const {
            SDL_Event sdlEvent{};
            switch (type) {
            case VXEventType::KeyDown:
            case VXEventType::KeyUp:
                sdlEvent.type = (type == VXEventType::KeyDown)
                                    ? SDL_EVENT_KEY_DOWN
                                    : SDL_EVENT_KEY_UP;
                sdlEvent.key.scancode = static_cast<SDL_Scancode>(
                    static_cast<uint32_t>(key.keyCode));
                sdlEvent.key.repeat = key.repeat ? 1 : 0;
                break;
            case VXEventType::MouseMove:
                sdlEvent.type = SDL_EVENT_MOUSE_MOTION;
                sdlEvent.motion.x = mouse.x;
                sdlEvent.motion.y = mouse.y;
                sdlEvent.motion.xrel = mouse.deltaX;
                sdlEvent.motion.yrel = mouse.deltaY;
                break;
            case VXEventType::MouseDown:
            case VXEventType::MouseUp:
                sdlEvent.type = (type == VXEventType::MouseDown)
                                    ? SDL_EVENT_MOUSE_BUTTON_DOWN
                                    : SDL_EVENT_MOUSE_BUTTON_UP;
                sdlEvent.button.button = mouse.button;
                sdlEvent.button.x = mouse.x;
                sdlEvent.button.y = mouse.y;
                break;
            case VXEventType::MouseWheel:
                sdlEvent.type = SDL_EVENT_MOUSE_WHEEL;
                sdlEvent.wheel.x = mouse.wheelX;
                sdlEvent.wheel.y = mouse.wheelY;
                break;
            case VXEventType::WindowFocusGained:
                sdlEvent.type = SDL_EVENT_WINDOW_FOCUS_GAINED;
                break;
            case VXEventType::WindowFocusLost:
                sdlEvent.type = SDL_EVENT_WINDOW_FOCUS_LOST;
                break;
            default: return sdlEvent;
            }

            return sdlEvent;
        }
    };
}

#endif //VXGE_EVENT_HPP

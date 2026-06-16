// src/VXWindow.cpp
#include <VXGE/VXWindow.hpp>
#include <VXGE/VXError.hpp>
#include <VXGE/VXEvent.hpp>

namespace VX {
    VXWindow::VXWindow(const std::string& title, const VXMonitor& monitor, VXFlags flags)
        : m_title(title), m_width(monitor.Width()), m_height(monitor.Height()), m_flags(flags) {
    }

    VXWindow::~VXWindow() {
        if (m_window)
            SDL_DestroyWindow(m_window);
    }

    bool VXWindow::Populate() {
        SDL_WindowFlags sdlFlags = SDL_WINDOW_VULKAN;

        if (m_flags & VXFlags::WINDOW_FULLSCREEN)
            sdlFlags |= SDL_WINDOW_FULLSCREEN;
        if (m_flags & VXFlags::WINDOW_BORDERLESS)
            sdlFlags |= SDL_WINDOW_BORDERLESS;
        if (!(m_flags & VXFlags::WINDOW_FULLSCREEN) && !(m_flags & VXFlags::WINDOW_BORDERLESS))
            sdlFlags |= SDL_WINDOW_RESIZABLE;

        m_window = SDL_CreateWindow(m_title.c_str(), m_width, m_height, sdlFlags);
        if (!m_window) {
            SetLastError(VXError(SDL_GetError()));
            return false;
        }

        return true;
    }

    bool VXWindow::IsClosing() const {
        return m_closing;
    }

    void VXWindow::PollEvents() {
        SDL_Event sdlEvent;
        while (SDL_PollEvent(&sdlEvent)) {
            VXEvent event{};

            switch (sdlEvent.type) {
            case SDL_EVENT_QUIT:
                event.type = VXEventType::Quit;
                if (!m_onCloseRequest || m_onCloseRequest())
                    m_closing = true;
                break;

            case SDL_EVENT_KEY_DOWN:
                event.type = VXEventType::KeyDown;
                event.key.keyCode = static_cast<VXKey>(sdlEvent.key.scancode);
                event.key.repeat = sdlEvent.key.repeat;
                if (m_hasCloseKey && static_cast<VXKey>(sdlEvent.key.scancode) == m_closeKey)
                    if (!m_onCloseRequest || m_onCloseRequest())
                        m_closing = true;
                break;

            case SDL_EVENT_KEY_UP:
                event.type = VXEventType::KeyUp;
                event.key.keyCode = static_cast<VXKey>(sdlEvent.key.scancode);
                event.key.repeat = false;
                break;

            case SDL_EVENT_MOUSE_MOTION:
                event.type = VXEventType::MouseMove;
                event.mouse.x = sdlEvent.motion.x;
                event.mouse.y = sdlEvent.motion.y;
                event.mouse.deltaX = sdlEvent.motion.xrel;
                event.mouse.deltaY = sdlEvent.motion.yrel;
                event.mouse.button = 0;
                break;

            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                event.type = VXEventType::MouseDown;
                event.mouse.x = sdlEvent.button.x;
                event.mouse.y = sdlEvent.button.y;
                event.mouse.button = sdlEvent.button.button;
                break;

            case SDL_EVENT_MOUSE_BUTTON_UP:
                event.type = VXEventType::MouseUp;
                event.mouse.x = sdlEvent.button.x;
                event.mouse.y = sdlEvent.button.y;
                event.mouse.button = sdlEvent.button.button;
                break;

            case SDL_EVENT_MOUSE_WHEEL:
                event.type = VXEventType::MouseWheel;
                event.mouse.wheelX = sdlEvent.wheel.x;
                event.mouse.wheelY = sdlEvent.wheel.y;
                break;

            case SDL_EVENT_AUDIO_DEVICE_ADDED:
                event.type = VXEventType::AudioDeviceAdded;
                event.audio.deviceId = sdlEvent.adevice.which;
                event.audio.isCapture = sdlEvent.adevice.recording;
                break;

            case SDL_EVENT_AUDIO_DEVICE_REMOVED:
                event.type = VXEventType::AudioDeviceRemoved;
                event.audio.deviceId = sdlEvent.adevice.which;
                event.audio.isCapture = sdlEvent.adevice.recording;
                break;

            case SDL_EVENT_GAMEPAD_ADDED:
                event.type = VXEventType::GamepadAdded;
                event.gamepad.gamepadId = sdlEvent.gdevice.which;
                break;

            case SDL_EVENT_GAMEPAD_REMOVED:
                event.type = VXEventType::GamepadRemoved;
                event.gamepad.gamepadId = sdlEvent.gdevice.which;
                break;

            case SDL_EVENT_GAMEPAD_AXIS_MOTION:
                event.type = VXEventType::GamepadAxis;
                event.gamepad.gamepadId = sdlEvent.gaxis.which;
                event.gamepad.axis = sdlEvent.gaxis.axis;
                event.gamepad.axisValue = sdlEvent.gaxis.value;
                break;

            case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
            case SDL_EVENT_GAMEPAD_BUTTON_UP:
                event.type = VXEventType::GamepadButton;
                event.gamepad.gamepadId = sdlEvent.gbutton.which;
                event.gamepad.button = sdlEvent.gbutton.button;
                break;

            case SDL_EVENT_WINDOW_RESIZED:
                event.type = VXEventType::WindowResized;
                event.window.width = sdlEvent.window.data1;
                event.window.height = sdlEvent.window.data2;
                break;

            case SDL_EVENT_WINDOW_FOCUS_GAINED:
                event.type = VXEventType::WindowFocusGained;
                break;

            case SDL_EVENT_WINDOW_FOCUS_LOST:
                event.type = VXEventType::WindowFocusLost;
                break;

            case SDL_EVENT_WINDOW_MINIMIZED:
                event.type = VXEventType::WindowMinimized;
                break;

            case SDL_EVENT_WINDOW_RESTORED:
                event.type = VXEventType::WindowRestored;
                break;

            default:
                continue;
            }

            if (m_onEvent)
                m_onEvent(event);
        }
    }

    const std::string& VXWindow::Title() const {
        return m_title;
    }

    uint32_t VXWindow::Width() const {
        return m_width;
    }

    uint32_t VXWindow::Height() const {
        return m_height;
    }

    SDL_Window* VXWindow::SDLWindow() const {
        return m_window;
    }

    VkSurfaceKHR VXWindow::CreateSurface(VkInstance instance) const {
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        if (!SDL_Vulkan_CreateSurface(m_window, instance, nullptr, &surface))
            SetLastError(VXError(SDL_GetError()));
        return surface;
    }

    void VXWindow::LockMouse() {
        SDL_SetWindowRelativeMouseMode(m_window, true);
        m_mouseLocked = true;
    }

    void VXWindow::UnlockMouse() {
        SDL_SetWindowRelativeMouseMode(m_window, false);
        m_mouseLocked = false;
    }

    bool VXWindow::IsMouseLocked() const {
        return m_mouseLocked;
    }

    void VXWindow::SetOnCloseRequest(std::function<bool()> callback) {
        m_onCloseRequest = callback;
    }

    void VXWindow::SetOnEvent(std::function<void(VXEvent&)> callback) {
        m_onEvent = callback;
    }

    bool VXWindow::IsKeyHeld(VXKey key) const {
        const bool* keys = SDL_GetKeyboardState(nullptr);
        return keys[static_cast<uint32_t>(key)];
    }

    void VXWindow::SetCloseKey(VXKey key) {
        m_closeKey = key;
        m_hasCloseKey = true;
    }

    void VXWindow::ClearCloseKey() {
        m_hasCloseKey = false;
    }
}

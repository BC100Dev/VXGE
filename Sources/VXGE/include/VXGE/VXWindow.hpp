#ifndef VXGE_WINDOW_HPP
#define VXGE_WINDOW_HPP

#include <functional>
#include <string>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include "VXFlags.hpp"
#include "VXMonitor.hpp"
#include "VXEvent.hpp"

namespace VX {
    class VXWindow {
    public:
        VXWindow() = default;
        VXWindow(const std::string& title, const VXMonitor& monitor, VXFlags flags);
        ~VXWindow();

        bool Populate();
        bool IsClosing() const;
        void PollEvents();
        const std::string& Title() const;
        uint32_t Width() const;
        uint32_t Height() const;
        SDL_Window* SDLWindow() const;
        VkSurfaceKHR CreateSurface(VkInstance instance) const;

        void SetOnCloseRequest(std::function<bool()> callback);
        void SetOnEvent(std::function<void(VXEvent&)> callback);
        void AddOnEvent(std::function<void(VXEvent&)> callback);

        void LockMouse();
        void UnlockMouse();
        bool IsMouseLocked() const;

        bool IsKeyHeld(VXKey key) const;
        void SetCloseKey(VXKey key);
        void ClearCloseKey();

        void Close();
        void FireCloseEvent();

    private:
        std::string m_title;
        int m_width = 0;
        int m_height = 0;
        VXFlags m_flags = VXFlags::NONE;
        SDL_Window* m_window = nullptr;
        bool m_closing = false;
        bool m_mouseLocked = false;
        VXKey m_closeKey = VXKey::Escape;
        bool m_hasCloseKey = false;

        std::function<bool()> m_onCloseRequest;
        std::function<void(VXEvent&)> m_onEvent;
    };
}

#endif //VXGE_WINDOW_HPP

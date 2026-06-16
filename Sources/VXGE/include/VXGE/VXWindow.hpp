#ifndef VXGE_WINDOW_HPP
#define VXGE_WINDOW_HPP

#include <string>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include "VXFlags.hpp"
#include "VXMonitor.hpp"

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

    private:
        std::string m_title;
        int m_width = 0;
        int m_height = 0;
        VXFlags m_flags = VXFlags::NONE;
        SDL_Window* m_window = nullptr;
        bool m_closing = false;
    };
}

#endif //VXGE_WINDOW_HPP

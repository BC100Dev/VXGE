// src/VXWindow.cpp
#include <VXGE/VXWindow.hpp>
#include <VXGE/VXError.hpp>

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
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT)
                m_closing = true;
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
}

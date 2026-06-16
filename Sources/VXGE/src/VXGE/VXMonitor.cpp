#include <VXGE/VXMonitor.hpp>
#include <VXGE/VXError.hpp>

namespace VX {

    VXMonitor::VXMonitor(SDL_DisplayID displayID) : m_displayID(displayID) {
        const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(displayID);
        if (!mode) {
            SetLastError(VXError("Invalid display ID"));
            return;
        }

        m_metrics.width = mode->w;
        m_metrics.height = mode->h;
        m_metrics.refreshRate = mode->refresh_rate;

        const char* name = SDL_GetDisplayName(displayID);
        m_name = name ? name : "";
    }

    SDL_DisplayID VXMonitor::DisplayID() const {
        return m_displayID;
    }

    int VXMonitor::Height() const {
        return m_metrics.height;
    }

    int VXMonitor::Width() const {
        return m_metrics.width;
    }

    float VXMonitor::RefreshRate() const {
        return m_metrics.refreshRate;
    }

    const std::string& VXMonitor::Name() const {
        return m_name;
    }
}

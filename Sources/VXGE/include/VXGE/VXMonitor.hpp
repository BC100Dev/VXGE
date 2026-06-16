#ifndef VXGE_MONITOR_HPP
#define VXGE_MONITOR_HPP

#include <SDL3/SDL.h>

#include <cstdint>
#include <string>

namespace VX {

    class VXMonitor {
    public:
        VXMonitor() = default;
        explicit VXMonitor(SDL_DisplayID displayID);

        int Width() const;
        int Height() const;
        float RefreshRate() const;
        const std::string& Name() const;
        SDL_DisplayID DisplayID() const;
    private:
        struct MMetrics {
            int width;
            int height;
            float refreshRate;
        };

        SDL_DisplayID m_displayID = 0;
        MMetrics m_metrics{};
        std::string m_name;
    };

}

#endif //VXGE_MONITOR_HPP

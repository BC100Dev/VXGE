#ifndef VXGE_HAPTIC_HPP
#define VXGE_HAPTIC_HPP

#include <SDL3/SDL.h>
#include <string>
#include <cstdint>

namespace VX {
    class VXHaptic {
    public:
        VXHaptic() = default;
        VXHaptic(uint32_t joystickId);
        ~VXHaptic();

        VXHaptic(const VXHaptic&) = delete;
        VXHaptic& operator=(const VXHaptic&) = delete;
        VXHaptic(VXHaptic&& other) noexcept;
        VXHaptic& operator=(VXHaptic&& other) noexcept;

        bool IsSupported() const;
        void Rumble(float strength, uint32_t durationMs);
        void Stop();
        const std::string& GetName() const;

    private:
        SDL_Haptic* m_haptic = nullptr;
        SDL_JoystickID m_joystickId = 0;
        std::string m_name;
        bool m_supported = false;
    };
}

#endif //VXGE_HAPTIC_HPP

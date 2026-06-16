#include <VXGE/VXHaptic.hpp>
#include <VXGE/VXError.hpp>

namespace VX {
    VXHaptic::VXHaptic(SDL_JoystickID joystickId)
        : m_joystickId(joystickId) {
        m_haptic = SDL_OpenHapticFromJoystick(SDL_OpenJoystick(joystickId));
        if (!m_haptic) {
            SetLastError(VXError(SDL_GetError()));
            return;
        }

        if (SDL_InitHapticRumble(m_haptic) != 0) {
            SetLastError(VXError(SDL_GetError()));
            SDL_CloseHaptic(m_haptic);
            m_haptic = nullptr;
            return;
        }

        const char* name = SDL_GetJoystickNameForID(joystickId);
        m_name = name ? name : "";
        m_supported = true;
    }

    VXHaptic::~VXHaptic() {
        if (m_haptic)
            SDL_CloseHaptic(m_haptic);
    }

    VXHaptic::VXHaptic(VXHaptic&& other) noexcept
        : m_haptic(other.m_haptic),
          m_joystickId(other.m_joystickId),
          m_name(std::move(other.m_name)),
          m_supported(other.m_supported) {
        other.m_haptic = nullptr;
        other.m_supported = false;
    }

    VXHaptic& VXHaptic::operator=(VXHaptic&& other) noexcept {
        if (this != &other) {
            if (m_haptic)
                SDL_CloseHaptic(m_haptic);

            m_haptic = other.m_haptic;
            m_joystickId = other.m_joystickId;
            m_name = std::move(other.m_name);
            m_supported = other.m_supported;
            other.m_haptic = nullptr;
            other.m_supported = false;
        }

        return *this;
    }

    bool VXHaptic::IsSupported() const { return m_supported; }

    void VXHaptic::Rumble(float strength, uint32_t durationMs) {
        if (!m_haptic || !m_supported) return;
        SDL_PlayHapticRumble(m_haptic, strength, durationMs);
    }

    void VXHaptic::Stop() {
        if (!m_haptic || !m_supported) return;
        SDL_StopHapticRumble(m_haptic);
    }

    const std::string& VXHaptic::GetName() const { return m_name; }
}

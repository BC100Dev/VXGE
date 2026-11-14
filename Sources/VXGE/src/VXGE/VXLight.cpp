#include <VXGE/VXLight.hpp>

namespace VX {
    void VXLight::SetDirection(float x, float y, float z) {
        m_direction = VXVec3(x, y, z).normalized();
    }

    void VXLight::SetDirection(const VXVec3& direction) {
        m_direction = direction.normalized();
    }

    VXVec3 VXLight::GetDirection() const {
        return m_direction;
    }

    void VXLight::SetColor(float r, float g, float b) {
        m_color = VXVec3(r, g, b);
    }

    void VXLight::SetColor(const VXVec3& color) {
        m_color = color;
    }

    VXVec3 VXLight::GetColor() const {
        return m_color;
    }

    void VXLight::SetIntensity(float intensity) {
        m_intensity = intensity;
    }

    float VXLight::GetIntensity() const {
        return m_intensity;
    }
}

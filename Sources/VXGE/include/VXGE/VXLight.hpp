#ifndef VXGE_LIGHT_HPP
#define VXGE_LIGHT_HPP

#include "VXMath.hpp"

namespace VX {
    class VXLight {
    public:
        VXLight() = default;

        void SetDirection(float x, float y, float z);
        void SetDirection(const VXVec3& direction);
        [[nodiscard]] VXVec3 GetDirection() const;

        void SetColor(float r, float g, float b);
        void SetColor(const VXVec3& color);
        [[nodiscard]] VXVec3 GetColor() const;

        void SetIntensity(float intensity);
        [[nodiscard]] float GetIntensity() const;

    private:
        VXVec3 m_direction = VXVec3(0.0f, -1.0f, 0.0f);
        VXVec3 m_color = VXVec3(1.0f, 1.0f, 1.0f);
        float m_intensity = 1.0f;
    };
}

#endif //VXGE_LIGHT_HPP

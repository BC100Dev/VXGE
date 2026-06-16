#include <VXGE/VXCamera.hpp>
#include <cmath>

namespace VX {
    static constexpr float PI = 3.14159265358979323846f;

    static float toRad(float deg) {
        return deg * PI / 180.0f;
    }

    VXCamera::VXCamera(float fovDeg, float aspect, float near, float far)
        : m_fov(toRad(fovDeg)), m_aspect(aspect), m_near(near), m_far(far) {
    }

    void VXCamera::SetPosition(const VXVec3& pos) {
        m_position = pos;
    }

    void VXCamera::SetRotation(float yaw, float pitch) {
        m_yaw = yaw;
        m_pitch = pitch;
    }

    void VXCamera::Move(const VXVec3& delta) { m_position += delta; }

    void VXCamera::Rotate(float dyaw, float dpitch) {
        m_yaw += dyaw;
        m_pitch += dpitch;
        if (m_pitch > 89.0f)
            m_pitch = 89.0f;

        if (m_pitch < -89.0f)
            m_pitch = -89.0f;
    }

    VXVec3 VXCamera::GetForward() const {
        return VXVec3(
            cosf(toRad(m_pitch)) * sinf(toRad(m_yaw)),
            -sinf(toRad(m_pitch)),
            cosf(toRad(m_pitch)) * cosf(toRad(m_yaw))
        ).normalized();
    }

    VXVec3 VXCamera::GetRight() const {
        VXVec3 up = {0.0f, 1.0f, 0.0f};
        return GetForward().cross(up).normalized();
    }

    VXMat4 VXCamera::GetView() const {
        VXVec3 forward = GetForward();
        VXVec3 center = m_position + forward;
        return VXMat4::LookAt(m_position, center, {0.0f, 1.0f, 0.0f});
    }

    VXMat4 VXCamera::GetProjection() const {
        return VXMat4::Perspective(m_fov, m_aspect, m_near, m_far);
    }

    VXMat4 VXCamera::GetVP() const {
        return GetProjection() * GetView();
    }

    const VXVec3& VXCamera::GetPosition() const {
        return m_position;
    }

    float VXCamera::GetYaw() const {
        return m_yaw;
    }

    float VXCamera::GetPitch() const {
        return m_pitch;
    }
}

#ifndef VXGE_CAMERA_HPP
#define VXGE_CAMERA_HPP

#include "VXMath.hpp"

namespace VX {
    class VXCamera {
    public:
        VXCamera() = default;
        VXCamera(float fovDeg, float aspect, float near, float far);

        void SetPosition(const VXVec3& pos);
        void SetRotation(float yaw, float pitch);
        void Move(const VXVec3& delta);
        void Rotate(float dyaw, float dpitch);

        VXMat4 GetView() const;
        VXMat4 GetProjection() const;
        VXMat4 GetVP() const;

        const VXVec3& GetPosition() const;
        float GetYaw() const;
        float GetPitch() const;
        VXVec3 GetForward() const;
        VXVec3 GetRight() const;

    private:
        VXVec3 m_position = {0.0f, 0.0f, -2.0f};
        float m_yaw = 0.0f;
        float m_pitch = 0.0f;
        float m_fov = 0.0f;
        float m_aspect = 0.0f;
        float m_near = 0.0f;
        float m_far = 0.0f;
    };
}

#endif //VXGE_CAMERA_HPP

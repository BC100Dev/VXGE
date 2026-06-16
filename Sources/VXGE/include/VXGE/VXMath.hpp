#ifndef VXGE_MATH_HPP
#define VXGE_MATH_HPP

#include <cmath>
#include <cstring>

namespace VX {
    struct VXVec3 {
        float x, y, z;

        VXVec3() : x(0), y(0), z(0) {
        }

        VXVec3(float x, float y, float z) : x(x), y(y), z(z) {
        }

        VXVec3 operator+(const VXVec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
        VXVec3 operator-(const VXVec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
        VXVec3 operator*(float s) const { return {x * s, y * s, z * s}; }

        VXVec3& operator+=(const VXVec3& o) {
            x += o.x;
            y += o.y;
            z += o.z;
            return *this;
        }

        VXVec3& operator-=(const VXVec3& o) {
            x -= o.x;
            y -= o.y;
            z -= o.z;
            return *this;
        }

        float dot(const VXVec3& o) const { return x * o.x + y * o.y + z * o.z; }

        VXVec3 cross(const VXVec3& o) const {
            return {
                y * o.z - z * o.y,
                z * o.x - x * o.z,
                x * o.y - y * o.x
            };
        }

        float length() const { return sqrtf(x * x + y * y + z * z); }

        VXVec3 normalized() const {
            float len = length();
            if (len == 0.0f) return {0, 0, 0};
            return {x / len, y / len, z / len};
        }
    };

    struct VXMat4 {
        float m[16] = {
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1
        };

        static VXMat4 Identity() {
            VXMat4 mat;
            return mat;
        }

        static VXMat4 Perspective(float fovRad, float aspect, float near, float far) {
            VXMat4 mat{};
            memset(mat.m, 0, sizeof(mat.m));
            float tanHalf = tanf(fovRad / 2.0f);
            mat.m[0] = 1.0f / (aspect * tanHalf);
            mat.m[5] = -1.0f / tanHalf; // flip Y for Vulkan
            mat.m[10] = far / (near - far);
            mat.m[11] = -1.0f;
            mat.m[14] = (near * far) / (near - far);
            return mat;
        }

        static VXMat4 LookAt(const VXVec3& eye, const VXVec3& center, const VXVec3& up) {
            VXVec3 f = (center - eye).normalized();
            VXVec3 r = f.cross(up).normalized();
            VXVec3 u = r.cross(f);

            VXMat4 mat{};
            memset(mat.m, 0, sizeof(mat.m));
            mat.m[0] = r.x;
            mat.m[4] = r.y;
            mat.m[8] = r.z;
            mat.m[1] = u.x;
            mat.m[5] = u.y;
            mat.m[9] = u.z;
            mat.m[2] = -f.x;
            mat.m[6] = -f.y;
            mat.m[10] = -f.z;
            mat.m[12] = -r.dot(eye);
            mat.m[13] = -u.dot(eye);
            mat.m[14] = f.dot(eye);
            mat.m[15] = 1.0f;
            return mat;
        }

        static VXMat4 Translation(const VXVec3& t) {
            VXMat4 mat;
            mat.m[12] = t.x;
            mat.m[13] = t.y;
            mat.m[14] = t.z;
            return mat;
        }

        VXMat4 operator*(const VXMat4& o) const {
            VXMat4 result{};
            memset(result.m, 0, sizeof(result.m));
            for (int col = 0; col < 4; col++)
                for (int row = 0; row < 4; row++)
                    for (int k = 0; k < 4; k++)
                        result.m[col * 4 + row] += m[k * 4 + row] * o.m[col * 4 + k];
            return result;
        }

        const float* Data() const { return m; }
    };
}

#endif //VXGE_MATH_HPP

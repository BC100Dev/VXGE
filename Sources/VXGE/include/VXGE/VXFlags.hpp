#ifndef VXGE_FLAGS_HPP
#define VXGE_FLAGS_HPP

#include <cstdint>

namespace VX {
    enum class VXFlags : uint64_t {
        NONE = 0,

        METRICS_SCREEN_DIMENS = 0x00100001,

        WINDOW_FULLSCREEN = 0x00200001,
        WINDOW_WINDOWED = 0x00200002,
        WINDOW_BORDERLESS = 0x00200003,

        RENDERER_2D = 0x00300001,
        RENDERER_3D = 0x00300002,

        CULL_NONE = 0x00400001,
        CULL_BACK = 0x00400002,
        CULL_FRONT = 0x00400003,
        CULL_FRONT_BACK = 0x00400004,

        MESH_TRANSPARENT =  0x00500001,
    };

    inline VXFlags operator|(VXFlags a, VXFlags b) {
        return static_cast<VXFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    inline bool operator&(VXFlags a, VXFlags b) {
        return (static_cast<uint32_t>(a) & 0x00FFFFFF) & (static_cast<uint32_t>(b) & 0x00FFFFFF);
    }
}

#endif //VXGE_FLAGS_HPP

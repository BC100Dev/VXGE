#ifndef VXGE_FLAGS_HPP
#define VXGE_FLAGS_HPP

#include <algorithm>
#include <cstdint>
#include <vector>

namespace VX {
    // Plain (non-bitmask) flag enum. Functions that used to take one OR'd VXFlags value now
    // take a std::vector<VXFlags> instead, so there is no need for these values to live in
    // non-overlapping bit groups any more; the numbering below is kept only for readability
    // and stability, not for bitwise combination.
    enum class VXFlags : uint64_t {
        NONE = 0,

        METRICS_SCREEN_DIMENS = 0x00100001,

        WINDOW_FULLSCREEN = 0x00200001,
        WINDOW_WINDOWED = 0x00200002,
        WINDOW_BORDERLESS = 0x00200003,

        CULL_NONE = 0x00400001,
        CULL_BACK = 0x00400002,
        CULL_FRONT = 0x00400003,
        CULL_FRONT_BACK = 0x00400004,

        MESH_TRANSPARENT =  0x00500001,
    };

    inline bool HasFlag(const std::vector<VXFlags>& flags, VXFlags flag) {
        return std::find(flags.begin(), flags.end(), flag) != flags.end();
    }
}

#endif //VXGE_FLAGS_HPP

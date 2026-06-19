#ifndef VXGE_OVERLAYDATA_HPP
#define VXGE_OVERLAYDATA_HPP

namespace VX {
    class VXOverlay;
    class VXRenderer;

    struct OverlayData {
        VXOverlay*  overlay  = nullptr;
        VXRenderer* renderer = nullptr;
    };

    extern OverlayData ovData;
}

#endif // VXGE_OVERLAYDATA_HPP

#ifndef VXGE_ENGINE_HPP
#define VXGE_ENGINE_HPP

#include <vector>
#include <string>
#include <vulkan/vulkan.h>
#include "VXError.hpp"
#include "VXOverlay.hpp"
#include "VXFlags.hpp"
#include "VXGraphics.hpp"
#include "VXMonitor.hpp"
#include "VXWindow.hpp"
#include "VXRenderer.hpp"
#include "VXHaptic.hpp"

namespace VX {
    class VXEngine {
    public:
        explicit VXEngine(const std::string& appName);
        ~VXEngine();

        bool Initialize();
        void Shutdown();

        std::vector<VXGraphics> EnumerateGraphicsCards();
        std::vector<VXMonitor> EnumerateMonitors();
        std::vector<VXHaptic> EnumerateHapticDevices();

        VXWindow CreateWindow(const std::string& title, const VXMonitor& monitor, VXFlags flags);
        VXRenderer CreateRenderer(VXWindow& window, VXGraphics& graphics, VXFlags flags);
        VXOverlay CreateOverlay(VXWindow& window, VXGraphics& graphics, VXRenderer& renderer);

        void ShowWindow(VXWindow& window);
        void LockWindowContext();

        void DestroyWindow(VXWindow& window);
        void DestroyRenderer(VXRenderer& renderer);
        void DestroyGraphics(std::vector<VXGraphics>& gpus);

        float DeltaTime() const;
        bool TickTime();

    private:
        std::string m_appName;
        VkInstance m_instance = VK_NULL_HANDLE;
        bool m_contextLocked = false;
        int vx_fpsCap = 60;
        bool vx_fpsCapEnabled = false;

        static const std::vector<const char*> VALIDATION_LAYERS;

#ifdef NDEBUG
        static constexpr bool ENABLE_VALIDATION = false;
#else
        static constexpr bool ENABLE_VALIDATION = true;
#endif

        void createInstance();
        bool s_shutdownInitiated = false;
    };
}

#endif //VXGE_ENGINE_HPP

#ifndef VXGE_ENGINE_HPP
#define VXGE_ENGINE_HPP

#include <vector>
#include <string>
#include <vulkan/vulkan.h>
#include "VXError.hpp"
#include "VXFlags.hpp"
#include "VXGraphics.hpp"
#include "VXMonitor.hpp"
#include "VXWindow.hpp"
#include "VXRenderer.hpp"

namespace VX {
    class VXEngine {
    public:
        VXEngine(const std::string& appName);
        ~VXEngine();

        bool Initialize();
        void Shutdown();

        std::vector<VXGraphics> EnumerateGraphicsCards();
        std::vector<VXMonitor> EnumerateMonitors();

        VXWindow CreateWindow(const std::string& title, const VXMonitor& monitor, VXFlags flags);
        VXRenderer CreateRenderer(VXWindow& window, VXGraphics& graphics, VXFlags flags);

        void ShowWindow(VXWindow& window);
        void LockWindowContext();
        void DestroyWindow(VXWindow& window);
        void DestroyRenderer(VXRenderer& renderer);

    private:
        std::string m_appName;
        VkInstance m_instance = VK_NULL_HANDLE;
        bool m_contextLocked = false;

        static const std::vector<const char*> VALIDATION_LAYERS;

#ifdef NDEBUG
        static constexpr bool ENABLE_VALIDATION = false;
#else
        static constexpr bool ENABLE_VALIDATION = true;
#endif

        void createInstance();
    };
}

#endif //VXGE_ENGINE_HPP

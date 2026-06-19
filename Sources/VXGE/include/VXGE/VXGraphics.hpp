#ifndef VXGE_GRAPHICS_HPP
#define VXGE_GRAPHICS_HPP

#include <vulkan/vulkan.h>

#include <string>
#include <vector>

namespace VX {
    class VXGraphics {
    public:
        VXGraphics() = default;
        explicit VXGraphics(VkPhysicalDevice physicalDevice);
        ~VXGraphics();
        void Destroy();

        VXGraphics(const VXGraphics&) = delete;
        VXGraphics& operator=(const VXGraphics&) = delete;
        VXGraphics(VXGraphics&& other) noexcept;
        VXGraphics& operator=(VXGraphics&& other) noexcept;

        const std::string& DeviceName() const;
        VkPhysicalDevice PhysicalDevice() const;
        VkDevice Device() const;
        VkQueue GraphicsQueue() const;
        VkQueue PresentQueue() const;
        uint32_t GraphicsFamily() const;
        uint32_t PresentFamily() const;

    private:
        VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
        VkDevice m_device = VK_NULL_HANDLE;
        VkQueue m_graphicsQueue = VK_NULL_HANDLE;
        VkQueue m_presentQueue = VK_NULL_HANDLE;
        uint32_t m_graphicsFamily = 0;
        uint32_t m_presentFamily = 0;
        std::string m_deviceName;

        static const std::vector<const char*> DEVICE_EXTENSIONS;

        bool findQueueFamilies();
        void createLogicalDevice();
        void ResetMovedFromState() noexcept;
    };
}

#endif //VXGE_GRAPHICS_HPP

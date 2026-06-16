#include <VXGE/VXGraphics.hpp>
#include <VXGE/VXError.hpp>

#include <cstring>
#include <vector>

namespace VX {
    const std::vector<const char*> VXGraphics::DEVICE_EXTENSIONS = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    VXGraphics::VXGraphics(VkPhysicalDevice physicalDevice)
        : m_physicalDevice(physicalDevice) {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(m_physicalDevice, &props);
        m_deviceName = props.deviceName;

        if (!findQueueFamilies()) {
            SetLastError(VXError("Failed to find suitable queue families"));
            return;
        }

        createLogicalDevice();
    }

    VXGraphics::~VXGraphics() {
        if (m_device != VK_NULL_HANDLE)
            vkDestroyDevice(m_device, nullptr);
    }

    bool VXGraphics::findQueueFamilies() {
        uint32_t count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &count, nullptr);
        std::vector<VkQueueFamilyProperties> families(count);
        vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &count, families.data());

        bool foundGraphics = false, foundPresent = false;

        for (uint32_t i = 0; i < count; i++) {
            if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                m_graphicsFamily = i;
                foundGraphics = true;
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(m_physicalDevice, i, VK_NULL_HANDLE, &presentSupport);
            if (presentSupport) {
                m_presentFamily = i;
                foundPresent = true;
            }

            if (foundGraphics && foundPresent) return true;
        }
        return false;
    }

    void VXGraphics::createLogicalDevice() {
        float priority = 1.0f;
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

        for (uint32_t family : {m_graphicsFamily, m_presentFamily}) {
            VkDeviceQueueCreateInfo qi{};
            qi.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            qi.queueFamilyIndex = family;
            qi.queueCount = 1;
            qi.pQueuePriorities = &priority;
            queueCreateInfos.push_back(qi);
        }

        VkPhysicalDeviceFeatures features{};
        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(DEVICE_EXTENSIONS.size());
        createInfo.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data();
        createInfo.pEnabledFeatures = &features;

        if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS) {
            SetLastError(VXError("Failed to create logical device"));
            return;
        }

        vkGetDeviceQueue(m_device, m_graphicsFamily, 0, &m_graphicsQueue);
        vkGetDeviceQueue(m_device, m_presentFamily, 0, &m_presentQueue);
    }

    const std::string& VXGraphics::DeviceName() const {
        return m_deviceName;
    }

    VkPhysicalDevice VXGraphics::PhysicalDevice() const {
        return m_physicalDevice;
    }

    VkDevice VXGraphics::Device() const {
        return m_device;
    }

    VkQueue VXGraphics::GraphicsQueue() const {
        return m_graphicsQueue;
    }

    VkQueue VXGraphics::PresentQueue() const {
        return m_presentQueue;
    }

    uint32_t VXGraphics::GraphicsFamily() const {
        return m_graphicsFamily;
    }

    uint32_t VXGraphics::PresentFamily() const {
        return m_presentFamily;
    }
}

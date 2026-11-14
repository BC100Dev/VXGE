#include <VXGE/VXTexture.hpp>

namespace VX {
    VXTexture::VXTexture(VkImage image, VkDeviceMemory memory, VkImageView view, VkSampler sampler,
                         uint32_t width, uint32_t height)
        : m_image(image), m_memory(memory), m_view(view), m_sampler(sampler),
          m_width(width), m_height(height) {
    }

    VkImage VXTexture::GetImage() const {
        return m_image;
    }

    VkImageView VXTexture::GetImageView() const {
        return m_view;
    }

    VkSampler VXTexture::GetSampler() const {
        return m_sampler;
    }

    uint32_t VXTexture::GetWidth() const {
        return m_width;
    }

    uint32_t VXTexture::GetHeight() const {
        return m_height;
    }

    void VXTexture::Destroy(VkDevice device) {
        if (m_view != VK_NULL_HANDLE)
            vkDestroyImageView(device, m_view, nullptr);
        if (m_image != VK_NULL_HANDLE)
            vkDestroyImage(device, m_image, nullptr);
        if (m_memory != VK_NULL_HANDLE)
            vkFreeMemory(device, m_memory, nullptr);

        m_sampler = VK_NULL_HANDLE;
        m_view = VK_NULL_HANDLE;
        m_image = VK_NULL_HANDLE;
        m_memory = VK_NULL_HANDLE;
    }
}

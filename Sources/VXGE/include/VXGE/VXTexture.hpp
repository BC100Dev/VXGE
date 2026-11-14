#ifndef VXGE_TEXTURE_HPP
#define VXGE_TEXTURE_HPP

#include <vulkan/vulkan.h>

namespace VX {
    class VXTexture {
    public:
        VXTexture() = default;
        VXTexture(VkImage image, VkDeviceMemory memory, VkImageView view, VkSampler sampler,
                  uint32_t width, uint32_t height);

        [[nodiscard]] VkImage GetImage() const;
        [[nodiscard]] VkImageView GetImageView() const;
        [[nodiscard]] VkSampler GetSampler() const;
        [[nodiscard]] uint32_t GetWidth() const;
        [[nodiscard]] uint32_t GetHeight() const;

        void Destroy(VkDevice device);

    private:
        VkImage m_image = VK_NULL_HANDLE;
        VkDeviceMemory m_memory = VK_NULL_HANDLE;
        VkImageView m_view = VK_NULL_HANDLE;
        VkSampler m_sampler = VK_NULL_HANDLE;
        uint32_t m_width = 0;
        uint32_t m_height = 0;
    };
}

#endif //VXGE_TEXTURE_HPP

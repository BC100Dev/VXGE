#ifndef VXGE_RENDERER_HPP
#define VXGE_RENDERER_HPP

#include <vulkan/vulkan.h>
#include <vector>
#include "VXFlags.hpp"
#include "VXWindow.hpp"
#include "VXGraphics.hpp"

namespace VX {
    class VXRenderer {
    public:
        VXRenderer() = default;
        VXRenderer(VXWindow& window, VXGraphics& graphics, VkInstance instance, VXFlags flags);
        ~VXRenderer();

        bool Populate();
        void Render();
        void Destroy();

    private:
        VXWindow*        m_window   = nullptr;
        VXGraphics*      m_graphics = nullptr;
        VkInstance       m_instance = VK_NULL_HANDLE;
        VXFlags          m_flags    = VXFlags::NONE;

        VkSurfaceKHR               m_surface         = VK_NULL_HANDLE;
        VkSwapchainKHR             m_swapchain        = VK_NULL_HANDLE;
        VkFormat                   m_swapchainFormat  = VK_FORMAT_UNDEFINED;
        VkExtent2D                 m_swapchainExtent  = {};
        VkRenderPass               m_renderPass       = VK_NULL_HANDLE;
        VkCommandPool              m_commandPool      = VK_NULL_HANDLE;
        uint32_t                   m_currentFrame     = 0;

        std::vector<VkImage>         m_swapchainImages;
        std::vector<VkImageView>     m_swapchainImageViews;
        std::vector<VkFramebuffer>   m_framebuffers;
        std::vector<VkCommandBuffer> m_commandBuffers;
        std::vector<VkSemaphore>     m_imageAvailableSemaphores;
        std::vector<VkSemaphore>     m_renderFinishedSemaphores;
        std::vector<VkFence>         m_inFlightFences;

        static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

        void createSwapchain();
        void createImageViews();
        void createRenderPass();
        void createFramebuffers();
        void createCommandPool();
        void createCommandBuffers();
        void createSyncObjects();
        void recordCommandBuffer(VkCommandBuffer cmd, uint32_t imageIndex);
    };
}

#endif //VXGE_RENDERER_HPP

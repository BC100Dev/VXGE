#ifndef VXGE_RENDERER_HPP
#define VXGE_RENDERER_HPP

#include <filesystem>
#include <vulkan/vulkan.h>
#include <vector>
#include <chrono>
#include "VXFlags.hpp"
#include "VXMesh.hpp"
#include "VXMaterial.hpp"
#include "VXMath.hpp"
#include "VXCamera.hpp"

namespace fs = std::filesystem;

namespace VX {
    class VXWindow;
    class VXGraphics;

    struct VXDrawCall {
        VXMesh* mesh = nullptr;
        VXMaterial* material = nullptr;
        float transform[16] = {};
    };

    class VXRenderer {
    public:
        VXRenderer() = default;
        VXRenderer(VXWindow& window, VXGraphics& graphics, VkInstance instance, VXFlags flags);
        ~VXRenderer();

        bool Populate();
        void Submit(VXMesh& mesh, VXMaterial& material, const float transform[16]);
        void Render();
        void Destroy();

        VXMesh CreateMesh(const std::vector<VXVertex>& vertices, const std::vector<uint32_t>& indices);
        VXMaterial CreateMaterial(const fs::path& vertShaderPath, const fs::path& fragShaderPath,
                                  VXFlags cullMode = VXFlags::CULL_BACK);

        void SetCamera(const VXCamera& camera);

        void SetTargetFPS(int fps);
        void ClearTargetFPS();
        void SetVSync(bool vsync);

        void SetClearColor(float r, float g, float b, float a);

        VkInstance GetInstance() const;
        VkRenderPass GetRenderPass() const;
        VkCommandBuffer GetCurrentCommandBuffer() const;

    private:
        VXWindow* m_window = nullptr;
        VXGraphics* m_graphics = nullptr;
        VkInstance m_instance = VK_NULL_HANDLE;
        VXFlags m_flags = VXFlags::NONE;

        VkSurfaceKHR m_surface = VK_NULL_HANDLE;
        VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
        VkFormat m_swapchainFormat = VK_FORMAT_UNDEFINED;
        VkExtent2D m_swapchainExtent = {};
        VkRenderPass m_renderPass = VK_NULL_HANDLE;
        VkCommandPool m_commandPool = VK_NULL_HANDLE;
        uint32_t m_currentFrame = 0;
        VkImage m_depthImage = VK_NULL_HANDLE;
        VkDeviceMemory m_depthImageMemory = VK_NULL_HANDLE;
        VkImageView m_depthImageView = VK_NULL_HANDLE;

        std::vector<VkImage> m_swapchainImages;
        std::vector<VkImageView> m_swapchainImageViews;
        std::vector<VkFramebuffer> m_framebuffers;
        std::vector<VkCommandBuffer> m_commandBuffers;
        std::vector<VkSemaphore> m_imageAvailableSemaphores;
        std::vector<VkSemaphore> m_renderFinishedSemaphores;
        std::vector<VkFence> m_inFlightFences;
        std::vector<VXDrawCall> m_drawQueue;

        VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
        VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
        std::vector<VkDescriptorSet> m_descriptorSets;
        std::vector<VkBuffer> m_uboBuffers;
        std::vector<VkDeviceMemory> m_uboMemory;
        std::vector<void*> m_uboMapped;

        static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

        std::chrono::high_resolution_clock::time_point m_frameStart;
        int m_fpsCap = 60;
        bool m_fpsCapEnabled = false;
        bool m_vsync = false;
        float m_clearColor[4] = {0.1f, 0.1f, 0.1f, 1.0f};

        void createSwapchain();
        void createImageViews();
        void createRenderPass();
        void createFramebuffers();
        void createCommandPool();
        void createCommandBuffers();
        void createSyncObjects();
        void createDepthResources();
        void recordCommandBuffer(VkCommandBuffer cmd, uint32_t imageIndex);

        void createDescriptorSetLayout();
        void createDescriptorPool();
        void createDescriptorSets();
        void updateUBO(const VXMat4& mvp);

        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
        void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                          VkMemoryPropertyFlags properties,
                          VkBuffer& buffer, VkDeviceMemory& memory);
        void copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);
        VkShaderModule loadShader(const fs::path& path);
    };
}

#endif //VXGE_RENDERER_HPP

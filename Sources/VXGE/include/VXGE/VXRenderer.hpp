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
#include "VXTexture.hpp"

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
        VXRenderer(VXWindow& window, VXGraphics& graphics, VkInstance instance);
        ~VXRenderer();

        bool Populate();
        void Submit(VXMesh& mesh, VXMaterial& material, const float transform[16]);
        void Render();
        void Destroy();

        VXMesh CreateMesh(const std::vector<VXVertex>& vertices, const std::vector<uint32_t>& indices);
        VXMaterial CreateMaterial(const fs::path& vertShaderPath, const fs::path& fragShaderPath,
                                  const std::vector<VXFlags>& flags = {VXFlags::CULL_BACK});
        VXTexture CreateTexture(const fs::path& imagePath);

        // Decodes an encoded image (PNG/JPG/etc.) read from an already-open POSIX file
        // descriptor at the given byte range, e.g. a texture packed inside an archive or
        // package file. Does not take ownership of or close the fd.
        VXTexture CreateTextureFromFD(int fd, size_t offset, size_t size);

        // Decodes an encoded image already sitting in memory, e.g. a glTF/GLB embedded
        // or base64 texture that was decoded by the model loader into a byte buffer.
        VXTexture CreateTextureFromMemory(const uint8_t* data, size_t size);
        VXTexture CreateTextureFromMemory(const std::vector<uint8_t>& data);

        // Allocates/updates this material's per-material descriptor set (set = 1) from its
        // currently assigned Albedo texture. Call after SetAlbedo() and before first Submit().
        void BindMaterialTextures(VXMaterial& material);

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

        // Set = 1: per-material texture bindings (currently just Albedo, binding 0).
        VkDescriptorSetLayout m_materialSetLayout = VK_NULL_HANDLE;
        VkDescriptorPool m_materialDescriptorPool = VK_NULL_HANDLE;
        VkSampler m_defaultSampler = VK_NULL_HANDLE;

        // 1x1 opaque white texture bound to materials with no Albedo assigned, so an
        // untextured material still samples something valid instead of garbage memory.
        VXTexture m_whiteTexture;

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

        void createMaterialSetLayout();
        void createMaterialDescriptorPool();
        void createDefaultSampler();

        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
        void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                          VkMemoryPropertyFlags properties,
                          VkBuffer& buffer, VkDeviceMemory& memory);
        void copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);
        VkShaderModule loadShader(const fs::path& path);

        void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
                         VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
                         VkImage& image, VkDeviceMemory& memory);
        // Shared upload path for CreateTexture/CreateTextureFromFD/CreateTextureFromMemory:
        // takes already-decoded RGBA8 pixels and produces a sampled, shader-read-only VXTexture.
        VXTexture createTextureFromPixels(const unsigned char* pixels, uint32_t width, uint32_t height);
        void transitionImageLayout(VkImage image, VkFormat format,
                                   VkImageLayout oldLayout, VkImageLayout newLayout);
        void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
        VkCommandBuffer beginOneTimeCommands();
        void endOneTimeCommands(VkCommandBuffer cmd);
    };
}

#endif //VXGE_RENDERER_HPP

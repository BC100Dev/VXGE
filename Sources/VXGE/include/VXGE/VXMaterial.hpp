#ifndef VXGE_MATERIAL_HPP
#define VXGE_MATERIAL_HPP

#include <vulkan/vulkan.h>
#include "VXTexture.hpp"

namespace VX {
    class VXMaterial {
    public:
        VXMaterial() = default;
        VXMaterial(VkPipeline pipeline, VkPipelineLayout layout, bool usesLights = false);

        [[nodiscard]] VkPipeline GetPipeline() const;
        [[nodiscard]] VkPipelineLayout GetPipelineLayout() const;

        void SetAlbedo(VXTexture* texture);
        void SetNormal(VXTexture* texture);
        void SetRoughnessMetallic(VXTexture* texture);
        void SetAO(VXTexture* texture);

        [[nodiscard]] VXTexture* GetAlbedo() const;
        [[nodiscard]] VXTexture* GetNormal() const;
        [[nodiscard]] VXTexture* GetRoughnessMetallic() const;
        [[nodiscard]] VXTexture* GetAO() const;

        [[nodiscard]] VkDescriptorSet GetDescriptorSet() const;
        void SetDescriptorSet(VkDescriptorSet set);

        [[nodiscard]] bool UsesLights() const;

        void Destroy(VkDevice device);

    private:
        VkPipeline m_pipeline = VK_NULL_HANDLE;
        VkPipelineLayout m_layout = VK_NULL_HANDLE;

        VXTexture* m_albedo = nullptr;
        VXTexture* m_normal = nullptr;
        VXTexture* m_roughnessMetallic = nullptr;
        VXTexture* m_ao = nullptr;

        // Per-material descriptor set carrying the albedo binding alongside the
        // per-frame UBO set. Allocated by VXRenderer::CreateMaterial once a texture is assigned.
        VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;

        bool m_usesLights = false;
    };
}

#endif //VXGE_MATERIAL_HPP

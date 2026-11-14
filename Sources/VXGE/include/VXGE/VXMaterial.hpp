#ifndef VXGE_MATERIAL_HPP
#define VXGE_MATERIAL_HPP

#include <vulkan/vulkan.h>
#include "VXTexture.hpp"

namespace VX {
    class VXMaterial {
    public:
        VXMaterial() = default;
        VXMaterial(VkPipeline pipeline, VkPipelineLayout layout);

        VkPipeline GetPipeline() const;
        VkPipelineLayout GetPipelineLayout() const;

        // Only Albedo is currently sampled by the default pipeline/descriptor layout.
        // Normal, RoughnessMetallic and AO slots are carried on the material now so loaders
        // and content pipelines can populate them ahead of the renderer wiring them up.
        void SetAlbedo(VXTexture* texture);
        void SetNormal(VXTexture* texture);
        void SetRoughnessMetallic(VXTexture* texture);
        void SetAO(VXTexture* texture);

        VXTexture* GetAlbedo() const;
        VXTexture* GetNormal() const;
        VXTexture* GetRoughnessMetallic() const;
        VXTexture* GetAO() const;

        VkDescriptorSet GetDescriptorSet() const;
        void SetDescriptorSet(VkDescriptorSet set);

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
    };
}

#endif //VXGE_MATERIAL_HPP

#include <VXGE/VXMaterial.hpp>

namespace VX {
    VXMaterial::VXMaterial(VkPipeline pipeline, VkPipelineLayout layout)
        : m_pipeline(pipeline), m_layout(layout) {
    }

    VkPipeline VXMaterial::GetPipeline() const {
        return m_pipeline;
    }

    VkPipelineLayout VXMaterial::GetPipelineLayout() const {
        return m_layout;
    }

    void VXMaterial::SetAlbedo(VXTexture* texture) {
        m_albedo = texture;
    }

    void VXMaterial::SetNormal(VXTexture* texture) {
        m_normal = texture;
    }

    void VXMaterial::SetRoughnessMetallic(VXTexture* texture) {
        m_roughnessMetallic = texture;
    }

    void VXMaterial::SetAO(VXTexture* texture) {
        m_ao = texture;
    }

    VXTexture* VXMaterial::GetAlbedo() const {
        return m_albedo;
    }

    VXTexture* VXMaterial::GetNormal() const {
        return m_normal;
    }

    VXTexture* VXMaterial::GetRoughnessMetallic() const {
        return m_roughnessMetallic;
    }

    VXTexture* VXMaterial::GetAO() const {
        return m_ao;
    }

    VkDescriptorSet VXMaterial::GetDescriptorSet() const {
        return m_descriptorSet;
    }

    void VXMaterial::SetDescriptorSet(VkDescriptorSet set) {
        m_descriptorSet = set;
    }

    void VXMaterial::Destroy(VkDevice device) {
        vkDestroyPipeline(device, m_pipeline, nullptr);
        vkDestroyPipelineLayout(device, m_layout, nullptr);
    }
}

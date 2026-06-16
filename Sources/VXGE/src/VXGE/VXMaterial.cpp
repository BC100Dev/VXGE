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

    void VXMaterial::Destroy(VkDevice device) {
        vkDestroyPipeline(device, m_pipeline, nullptr);
        vkDestroyPipelineLayout(device, m_layout, nullptr);
    }
}

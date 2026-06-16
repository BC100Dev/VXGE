#ifndef VXGE_MATERIAL_HPP
#define VXGE_MATERIAL_HPP

#include <vulkan/vulkan.h>

namespace VX {
    class VXMaterial {
    public:
        VXMaterial() = default;
        VXMaterial(VkPipeline pipeline, VkPipelineLayout layout);

        VkPipeline GetPipeline() const;
        VkPipelineLayout GetPipelineLayout() const;
        void Destroy(VkDevice device);

    private:
        VkPipeline m_pipeline = VK_NULL_HANDLE;
        VkPipelineLayout m_layout = VK_NULL_HANDLE;
    };
}

#endif //VXGE_MATERIAL_HPP

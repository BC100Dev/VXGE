#ifndef VXGE_MESH_HPP
#define VXGE_MESH_HPP

#include <vulkan/vulkan.h>
#include <vector>

namespace VX {
    struct VXVertex {
        float position[3];
        float color[3];
    };

    class VXMesh {
    public:
        VXMesh() = default;
        VXMesh(VkBuffer vertexBuffer, VkDeviceMemory vertexMemory,
               VkBuffer indexBuffer, VkDeviceMemory indexMemory,
               uint32_t indexCount);

        VkBuffer GetVertexBuffer() const;
        VkBuffer GetIndexBuffer() const;
        uint32_t GetIndexCount() const;
        void Destroy(VkDevice device);

    private:
        VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory m_vertexMemory = VK_NULL_HANDLE;
        VkBuffer m_indexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory m_indexMemory = VK_NULL_HANDLE;
        uint32_t m_indexCount = 0;
    };
}

#endif //VXGE_MESH_HPP

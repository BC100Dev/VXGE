#ifndef VXGE_MESH_HPP
#define VXGE_MESH_HPP

#include <vulkan/vulkan.h>
#include <vector>

namespace VX {
    struct VXVertex {
        float position[3]{};
        float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        float uv[2] = {0.0f, 0.0f};
        float normal[3] = {0.0f, 0.0f, 0.0f};
    };

    class VXMesh {
    public:
        VXMesh() = default;
        VXMesh(VkBuffer vertexBuffer, VkDeviceMemory vertexMemory,
               VkBuffer indexBuffer, VkDeviceMemory indexMemory,
               uint32_t indexCount);

        [[nodiscard]] VkBuffer GetVertexBuffer() const;
        [[nodiscard]] VkBuffer GetIndexBuffer() const;
        [[nodiscard]] uint32_t GetIndexCount() const;
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

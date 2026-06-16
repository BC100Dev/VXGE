#include <VXGE/VXMesh.hpp>

namespace VX {
    VXMesh::VXMesh(VkBuffer vertexBuffer, VkDeviceMemory vertexMemory,
                   VkBuffer indexBuffer, VkDeviceMemory indexMemory,
                   uint32_t indexCount)
        : m_vertexBuffer(vertexBuffer), m_vertexMemory(vertexMemory),
          m_indexBuffer(indexBuffer), m_indexMemory(indexMemory),
          m_indexCount(indexCount) {
    }

    VkBuffer VXMesh::GetVertexBuffer() const {
        return m_vertexBuffer;
    }

    VkBuffer VXMesh::GetIndexBuffer() const {
        return m_indexBuffer;
    }

    uint32_t VXMesh::GetIndexCount() const {
        return m_indexCount;
    }

    void VXMesh::Destroy(VkDevice device) {
        vkDestroyBuffer(device, m_indexBuffer, nullptr);
        vkFreeMemory(device, m_indexMemory, nullptr);
        vkDestroyBuffer(device, m_vertexBuffer, nullptr);
        vkFreeMemory(device, m_vertexMemory, nullptr);
    }
}

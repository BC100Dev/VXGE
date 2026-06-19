#ifndef VXGE_MODEL_GLTF_HPP
#define VXGE_MODEL_GLTF_HPP

#include <filesystem>
#include <string>
#include <vector>
#include <cstdint>
#include <VXGE/JSON/json.hpp>
#include <VXGE/VXMesh.hpp>

namespace fs = std::filesystem;

namespace VX {
    class GLTFModel {
    public:
        GLTFModel() = default;

        struct MeshData {
            std::vector<VXVertex> vertices;
            std::vector<uint32_t> indices;
            std::string name;
        };

        bool Load(const fs::path& path);
        std::vector<MeshData>& GetMeshes();
        bool IsLoaded() const;

    private:
        std::vector<MeshData> m_meshes;
        bool m_loaded = false;

        bool parseGLTF(const fs::path& path);
        bool parseGLB(const fs::path& path);

        bool extractAccessor(const nlohmann::json& root, int accessorIdx,
                             const std::vector<uint8_t>& binData,
                             std::vector<float>& outFloats,
                             std::vector<uint32_t>& outInts,
                             bool isIndex);

        bool processMeshes(const nlohmann::json& root, const std::vector<uint8_t>& binData);
    };
}

#endif //VXGE_MODEL_GLTF_HPP

#ifndef VXGE_MODEL_GLTF_HPP
#define VXGE_MODEL_GLTF_HPP

#include <filesystem>
#include <string>
#include <vector>
#include <VXGE/JSON/json.hpp>
#include <VXGE/VXMesh.hpp>
#include <VXGE/VXMath.hpp>

namespace fs = std::filesystem;

namespace VX {
    class GLTFModel {
    public:
        GLTFModel() = default;

        struct MaterialRef {
            bool hasTexture = false;
            fs::path path;
            std::vector<uint8_t> bytes;
            float baseColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        };

        struct MeshData {
            std::vector<VXVertex> vertices;
            std::vector<uint32_t> indices;
            std::string name;
            VXMat4 transform;

            MaterialRef material;
        };

        bool Load(const fs::path& path);
        std::vector<MeshData>& GetMeshes();
        [[nodiscard]] bool IsLoaded() const;

    private:
        std::vector<MeshData> m_meshes;
        bool m_loaded = false;
        fs::path m_basePath;

        bool parseGLTF(const fs::path& path);
        bool parseGLB(const fs::path& path);

        bool extractAccessor(const nlohmann::json& root, int accessorIdx,
                             const std::vector<uint8_t>& binData,
                             std::vector<float>& outFloats,
                             std::vector<uint32_t>& outInts,
                             bool isIndex);

        bool processMeshes(const nlohmann::json& root, const std::vector<uint8_t>& binData);

        MaterialRef resolveMaterial(const nlohmann::json& root, int materialIdx,
                                    const std::vector<uint8_t>& binData);

        VXMat4 nodeLocalTransform(const nlohmann::json& node);

        void computeNodeWorldTransforms(const nlohmann::json& root,
                                        std::vector<VXMat4>& outMeshTransformByMeshIdx);
    };
}

#endif //VXGE_MODEL_GLTF_HPP

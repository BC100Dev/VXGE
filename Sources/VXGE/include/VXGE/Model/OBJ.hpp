#ifndef VXGE_MODEL_OBJ_HPP
#define VXGE_MODEL_OBJ_HPP

#include <string>
#include <vector>
#include <filesystem>

#include <VXGE/VXMesh.hpp>

namespace fs = std::filesystem;

namespace VX {
    class OBJModel {
    public:
        OBJModel() = default;

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

        bool parse(const fs::path& path);
    };
}

#endif //VXGE_MODEL_OBJ_HPP

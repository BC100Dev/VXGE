// src/Model/GLTF.cpp
#include <VXGE/Model/GLTF.hpp>
#include <VXGE/VXError.hpp>
#include <VXGE/JSON/json.hpp>
#include <fstream>
#include <cstring>

using json = nlohmann::json;

namespace VX {
    std::vector<GLTFModel::MeshData>& GLTFModel::GetMeshes() { return m_meshes; }
    bool GLTFModel::IsLoaded() const { return m_loaded; }

    bool GLTFModel::Load(const fs::path& path) {
        m_meshes.clear();
        m_loaded = false;

        if (path.extension() == ".glb")
            return parseGLB(path);

        return parseGLTF(path);
    }

    bool GLTFModel::extractAccessor(const json& root, int accessorIdx,
                                 const std::vector<uint8_t>& binData,
                                 std::vector<float>& outFloats,
                                 std::vector<uint32_t>& outInts,
                                 bool isIndex) {
        const auto& accessors = root["accessors"];
        const auto& bufferViews = root["bufferViews"];

        if (accessorIdx < 0 || accessorIdx >= (int)accessors.size()) return false;

        const auto& acc = accessors[accessorIdx];
        int bufViewIdx = acc["bufferView"].get<int>();
        int count = acc["count"].get<int>();
        int compType = acc["componentType"].get<int>();
        int byteOffset = acc.value("byteOffset", 0);
        std::string typeStr = acc["type"].get<std::string>();

        int numComponents = 1;
        if (typeStr == "VEC2") numComponents = 2;
        else if (typeStr == "VEC3") numComponents = 3;
        else if (typeStr == "VEC4") numComponents = 4;
        else if (typeStr == "MAT4") numComponents = 16;

        const auto& bv = bufferViews[bufViewIdx];
        int bvOffset = bv.value("byteOffset", 0);
        int totalOffset = bvOffset + byteOffset;

        if (isIndex) {
            for (int i = 0; i < count; i++) {
                const uint8_t* ptr = binData.data() + totalOffset;
                if (compType == 5121) outInts.push_back(ptr[i]);
                else if (compType == 5123) outInts.push_back(reinterpret_cast<const uint16_t*>(ptr)[i]);
                else if (compType == 5125) outInts.push_back(reinterpret_cast<const uint32_t*>(ptr)[i]);
            }
        } else {
            for (int i = 0; i < count * numComponents; i++) {
                const uint8_t* ptr = binData.data() + totalOffset;
                if (compType == 5126)
                    outFloats.push_back(reinterpret_cast<const float*>(ptr)[i]);
            }
        }

        return true;
    }

    bool GLTFModel::processMeshes(const json& root, const std::vector<uint8_t>& binData) {
        if (!root.contains("meshes")) {
            SetLastError(VXError("glTF has no meshes"));
            return false;
        }

        for (const auto& meshNode : root["meshes"]) {
            std::string meshName = meshNode.value("name", "mesh");

            for (const auto& primitive : meshNode["primitives"]) {
                MeshData mesh;
                mesh.name = meshName;

                const auto& attrs = primitive["attributes"];

                std::vector<float> positions, normals;
                std::vector<uint32_t> indices;

                if (attrs.contains("POSITION"))
                    extractAccessor(root, attrs["POSITION"].get<int>(), binData, positions, indices, false);

                if (attrs.contains("NORMAL"))
                    extractAccessor(root, attrs["NORMAL"].get<int>(), binData, normals, indices, false);

                if (primitive.contains("indices")) {
                    std::vector<float> dummy;
                    extractAccessor(root, primitive["indices"].get<int>(), binData, dummy, indices, true);
                }

                int vertCount = (int)(positions.size() / 3);
                for (int i = 0; i < vertCount; i++) {
                    VXVertex v{};
                    v.position[0] = positions[i * 3 + 0];
                    v.position[1] = positions[i * 3 + 1];
                    v.position[2] = positions[i * 3 + 2];

                    if (!normals.empty() && (i * 3 + 2) < (int)normals.size()) {
                        v.color[0] = normals[i * 3 + 0] * 0.5f + 0.5f;
                        v.color[1] = normals[i * 3 + 1] * 0.5f + 0.5f;
                        v.color[2] = normals[i * 3 + 2] * 0.5f + 0.5f;
                    } else {
                        v.color[0] = v.color[1] = v.color[2] = 1.0f;
                    }

                    mesh.vertices.push_back(v);
                }

                mesh.indices = indices;

                if (!mesh.vertices.empty())
                    m_meshes.push_back(mesh);
            }
        }

        m_loaded = !m_meshes.empty();
        return m_loaded;
    }

    bool GLTFModel::parseGLTF(const fs::path& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            SetLastError(VXError("Failed to open glTF file: " + path.string()));
            return false;
        }

        json root;
        try {
            file >> root;
        } catch (const json::exception& e) {
            SetLastError(VXError(std::string("Failed to parse glTF JSON: ") + e.what()));
            return false;
        }
        file.close();

        std::vector<uint8_t> binData;
        if (root.contains("buffers") && !root["buffers"].empty()) {
            const auto& buf = root["buffers"][0];
            if (buf.contains("uri")) {
                fs::path binPath = path.parent_path() / buf["uri"].get<std::string>();
                std::ifstream binFile(binPath, std::ios::binary);
                if (binFile.is_open())
                    binData = std::vector<uint8_t>((std::istreambuf_iterator<char>(binFile)),
                                                   std::istreambuf_iterator<char>());
            }
        }

        return processMeshes(root, binData);
    }

    bool GLTFModel::parseGLB(const fs::path& path) {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            SetLastError(VXError("Failed to open GLB file: " + path.string()));
            return false;
        }

        uint32_t magic, version, totalLength;
        file.read(reinterpret_cast<char*>(&magic), 4);
        file.read(reinterpret_cast<char*>(&version), 4);
        file.read(reinterpret_cast<char*>(&totalLength), 4);

        if (magic != 0x46546C67) {
            SetLastError(VXError("Invalid GLB magic number: " + path.string()));
            return false;
        }

        std::string jsonStr;
        std::vector<uint8_t> binData;

        while ((uint32_t)file.tellg() < totalLength) {
            uint32_t chunkLength, chunkType;
            file.read(reinterpret_cast<char*>(&chunkLength), 4);
            file.read(reinterpret_cast<char*>(&chunkType), 4);

            std::vector<uint8_t> chunkData(chunkLength);
            file.read(reinterpret_cast<char*>(chunkData.data()), chunkLength);

            if (chunkType == 0x4E4F534A) jsonStr = std::string(chunkData.begin(), chunkData.end());
            else if (chunkType == 0x004E4942) binData = chunkData;
        }

        if (jsonStr.empty()) {
            SetLastError(VXError("GLB has no JSON chunk: " + path.string()));
            return false;
        }

        json root;
        try {
            root = json::parse(jsonStr);
        } catch (const json::exception& e) {
            SetLastError(VXError(std::string("Failed to parse GLB JSON: ") + e.what()));
            return false;
        }

        return processMeshes(root, binData);
    }
}

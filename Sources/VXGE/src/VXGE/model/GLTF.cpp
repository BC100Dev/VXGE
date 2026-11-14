// src/Model/GLTF.cpp
#include <VXGE/Model/GLTF.hpp>
#include <VXGE/VXError.hpp>
#include <VXGE/JSON/json.hpp>
#include <fstream>
#include <cstring>
#include <cmath>
#include <unordered_map>
#include <functional>
#include <algorithm>

using json = nlohmann::json;

namespace VX {
    namespace {
        // Minimal base64 decoder, only needed for data: URIs on images/buffers.
        std::vector<uint8_t> decodeBase64(const std::string& data) {
            static const std::string chars =
                "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
            std::vector<int> table(256, -1);
            for (size_t i = 0; i < chars.size(); i++)
                table[static_cast<unsigned char>(chars[i])] = static_cast<int>(i);

            std::vector<uint8_t> out;
            int val = 0, bits = -8;
            for (unsigned char c : data) {
                if (c == '=' || c == '\n' || c == '\r') continue;
                if (table[c] == -1) continue;
                val = (val << 6) + table[c];
                bits += 6;
                if (bits >= 0) {
                    out.push_back(static_cast<uint8_t>((val >> bits) & 0xFF));
                    bits -= 8;
                }
            }
            return out;
        }

        // "data:image/png;base64,..." / "data:application/octet-stream;base64,..." -> decoded bytes.
        // Returns empty if uri isn't a data-URI.
        std::vector<uint8_t> tryDecodeDataUri(const std::string& uri) {
            const std::string marker = ";base64,";
            if (uri.rfind("data:", 0) != 0) return {};
            size_t pos = uri.find(marker);
            if (pos == std::string::npos) return {};
            return decodeBase64(uri.substr(pos + marker.size()));
        }

        // Quaternion (x,y,z,w) -> 4x4 rotation matrix, column-major to match VXMat4.
        VXMat4 quatToMat4(float x, float y, float z, float w) {
            VXMat4 m{};
            float xx = x * x, yy = y * y, zz = z * z;
            float xy = x * y, xz = x * z, yz = y * z;
            float wx = w * x, wy = w * y, wz = w * z;

            m.m[0]  = 1.0f - 2.0f * (yy + zz);
            m.m[1]  = 2.0f * (xy + wz);
            m.m[2]  = 2.0f * (xz - wy);
            m.m[3]  = 0.0f;

            m.m[4]  = 2.0f * (xy - wz);
            m.m[5]  = 1.0f - 2.0f * (xx + zz);
            m.m[6]  = 2.0f * (yz + wx);
            m.m[7]  = 0.0f;

            m.m[8]  = 2.0f * (xz + wy);
            m.m[9]  = 2.0f * (yz - wx);
            m.m[10] = 1.0f - 2.0f * (xx + yy);
            m.m[11] = 0.0f;

            m.m[12] = 0.0f;
            m.m[13] = 0.0f;
            m.m[14] = 0.0f;
            m.m[15] = 1.0f;
            return m;
        }

        VXMat4 scaleMat4(float sx, float sy, float sz) {
            VXMat4 m{};
            memset(m.m, 0, sizeof(m.m));
            m.m[0] = sx;
            m.m[5] = sy;
            m.m[10] = sz;
            m.m[15] = 1.0f;
            return m;
        }

        // glTF accessor componentType constants (section 3.6.2.2 of the spec).
        constexpr int COMP_UBYTE  = 5121;
        constexpr int COMP_USHORT = 5123;
        constexpr int COMP_UINT   = 5125;
        constexpr int COMP_FLOAT  = 5126;
    }

    std::vector<GLTFModel::MeshData>& GLTFModel::GetMeshes() { return m_meshes; }
    bool GLTFModel::IsLoaded() const { return m_loaded; }

    bool GLTFModel::Load(const fs::path& path) {
        m_meshes.clear();
        m_loaded = false;
        m_basePath = path.parent_path();

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

        auto componentSize = [&](int ct) -> int {
            switch (ct) {
                case COMP_UBYTE:  return 1;
                case COMP_USHORT: return 2;
                case COMP_UINT:
                case COMP_FLOAT:  return 4;
                default: return 4;
            }
        };

        // byteStride, when present, is the distance between consecutive *elements*
        // (i.e. whole VEC2/VEC3/... groups) in an interleaved buffer view. When absent,
        // elements are tightly packed: stride == numComponents * componentSize.
        int tightStride = numComponents * componentSize(compType);
        int stride = bv.value("byteStride", tightStride);

        if (isIndex) {
            for (int i = 0; i < count; i++) {
                int elemOffset = totalOffset + i * stride;
                if (elemOffset < 0 || elemOffset >= (int)binData.size()) { outInts.push_back(0); continue; }
                const uint8_t* ptr = binData.data() + elemOffset;
                if (compType == COMP_UBYTE) outInts.push_back(ptr[0]);
                else if (compType == COMP_USHORT) outInts.push_back(*reinterpret_cast<const uint16_t*>(ptr));
                else if (compType == COMP_UINT) outInts.push_back(*reinterpret_cast<const uint32_t*>(ptr));
            }
        } else {
            for (int i = 0; i < count; i++) {
                int elemOffset = totalOffset + i * stride;
                for (int c = 0; c < numComponents; c++) {
                    int compOffset = elemOffset + c * componentSize(compType);
                    if (compOffset < 0 || compOffset + 4 > (int)binData.size()) { outFloats.push_back(0.0f); continue; }
                    const uint8_t* ptr = binData.data() + compOffset;
                    if (compType == COMP_FLOAT)
                        outFloats.push_back(*reinterpret_cast<const float*>(ptr));
                    else
                        outFloats.push_back(0.0f);
                }
            }
        }

        return true;
    }

    GLTFModel::MaterialRef GLTFModel::resolveMaterial(const json& root, int materialIdx,
                                                       const std::vector<uint8_t>& binData) {
        MaterialRef ref;
        if (materialIdx < 0 || !root.contains("materials")) return ref;

        const auto& materials = root["materials"];
        if (materialIdx >= (int)materials.size()) return ref;

        const auto& mat = materials[materialIdx];

        // Core glTF 2.0 metallic-roughness workflow: materials.pbrMetallicRoughness.baseColorTexture
        // / baseColorFactor.
        int texIdx = -1;
        if (mat.contains("pbrMetallicRoughness")) {
            const auto& pbr = mat["pbrMetallicRoughness"];
            if (pbr.contains("baseColorTexture"))
                texIdx = pbr["baseColorTexture"].value("index", -1);
            if (pbr.contains("baseColorFactor")) {
                const auto& bcf = pbr["baseColorFactor"];
                for (int c = 0; c < 4 && c < (int)bcf.size(); c++)
                    ref.baseColor[c] = bcf[c].get<float>();
            }
        }

        // Fallback: KHR_materials_pbrSpecularGlossiness.diffuseTexture / diffuseFactor. Older
        // DCC exports (3ds Max/Maya glTF exporters of this model's era) commonly use this
        // extension instead of the core workflow, with no baseColorTexture/Factor at all.
        if (mat.contains("extensions")) {
            const auto& exts = mat["extensions"];
            if (exts.contains("KHR_materials_pbrSpecularGlossiness")) {
                const auto& sg = exts["KHR_materials_pbrSpecularGlossiness"];
                if (texIdx < 0 && sg.contains("diffuseTexture"))
                    texIdx = sg["diffuseTexture"].value("index", -1);
                if (sg.contains("diffuseFactor")) {
                    const auto& df = sg["diffuseFactor"];
                    for (int c = 0; c < 4 && c < (int)df.size(); c++)
                        ref.baseColor[c] = df[c].get<float>();
                }
            }
        }

        if (texIdx < 0 || !root.contains("textures")) return ref;
        const auto& textures = root["textures"];
        if (texIdx >= (int)textures.size()) return ref;

        int imgIdx = textures[texIdx].value("source", -1);
        if (imgIdx < 0 || !root.contains("images")) return ref;
        const auto& images = root["images"];
        if (imgIdx >= (int)images.size()) return ref;

        const auto& img = images[imgIdx];

        if (img.contains("uri")) {
            std::string uri = img["uri"].get<std::string>();
            std::vector<uint8_t> embedded = tryDecodeDataUri(uri);
            if (!embedded.empty()) {
                ref.hasTexture = true;
                ref.bytes = std::move(embedded);
                return ref;
            }

            // Plain relative/absolute file reference alongside the .gltf.
            ref.hasTexture = true;
            ref.path = m_basePath / uri;
            return ref;
        }

        if (img.contains("bufferView")) {
            int bvIdx = img["bufferView"].get<int>();
            if (!root.contains("bufferViews")) return ref;
            const auto& bufferViews = root["bufferViews"];
            if (bvIdx < 0 || bvIdx >= (int)bufferViews.size()) return ref;

            const auto& bv = bufferViews[bvIdx];
            int offset = bv.value("byteOffset", 0);
            int length = bv.value("byteLength", 0);
            if (offset < 0 || length <= 0 || offset + length > (int)binData.size()) return ref;

            ref.hasTexture = true;
            ref.bytes.assign(binData.begin() + offset, binData.begin() + offset + length);
            return ref;
        }

        return ref;
    }

    VXMat4 GLTFModel::nodeLocalTransform(const json& node) {
        if (node.contains("matrix")) {
            VXMat4 m{};
            const auto& arr = node["matrix"];
            for (int i = 0; i < 16 && i < (int)arr.size(); i++)
                m.m[i] = arr[i].get<float>();
            return m;
        }

        VXMat4 t = VXMat4::Identity();
        if (node.contains("translation")) {
            const auto& tr = node["translation"];
            t = VXMat4::Translation(VXVec3(tr[0].get<float>(), tr[1].get<float>(), tr[2].get<float>()));
        }

        VXMat4 r = VXMat4::Identity();
        if (node.contains("rotation")) {
            const auto& rot = node["rotation"];
            r = quatToMat4(rot[0].get<float>(), rot[1].get<float>(), rot[2].get<float>(), rot[3].get<float>());
        }

        VXMat4 s = VXMat4::Identity();
        if (node.contains("scale")) {
            const auto& sc = node["scale"];
            s = scaleMat4(sc[0].get<float>(), sc[1].get<float>(), sc[2].get<float>());
        }

        // glTF TRS composition order: T * R * S.
        return t * r * s;
    }

    void GLTFModel::computeNodeWorldTransforms(const json& root,
                                               std::vector<VXMat4>& outMeshTransformByMeshIdx) {
        if (!root.contains("nodes") || !root.contains("meshes")) return;

        const auto& nodes = root["nodes"];
        size_t meshCount = root["meshes"].size();
        outMeshTransformByMeshIdx.assign(meshCount, VXMat4::Identity());

        std::vector<bool> visited(nodes.size(), false);

        // Recursive walk from every root node (found via scenes[scene].nodes, or -- if
        // that's absent -- treat every node not referenced as a child as a root).
        std::vector<int> roots;
        if (root.contains("scenes") && root.contains("scene")) {
            int sceneIdx = root["scene"].get<int>();
            if (sceneIdx >= 0 && sceneIdx < (int)root["scenes"].size()) {
                const auto& scene = root["scenes"][sceneIdx];
                if (scene.contains("nodes"))
                    for (const auto& n : scene["nodes"]) roots.push_back(n.get<int>());
            }
        }
        if (roots.empty()) {
            std::vector<bool> isChild(nodes.size(), false);
            for (const auto& n : nodes)
                if (n.contains("children"))
                    for (const auto& c : n["children"])
                        if (c.get<int>() < (int)isChild.size()) isChild[c.get<int>()] = true;
            for (size_t i = 0; i < nodes.size(); i++)
                if (!isChild[i]) roots.push_back((int)i);
        }

        std::function<void(int, const VXMat4&)> visit = [&](int nodeIdx, const VXMat4& parentWorld) {
            if (nodeIdx < 0 || nodeIdx >= (int)nodes.size() || visited[nodeIdx]) return;
            visited[nodeIdx] = true;

            const auto& node = nodes[nodeIdx];
            VXMat4 world = parentWorld * nodeLocalTransform(node);

            if (node.contains("mesh")) {
                int meshIdx = node["mesh"].get<int>();
                if (meshIdx >= 0 && meshIdx < (int)outMeshTransformByMeshIdx.size())
                    outMeshTransformByMeshIdx[meshIdx] = world;
            }

            if (node.contains("children"))
                for (const auto& c : node["children"])
                    visit(c.get<int>(), world);
        };

        for (int r : roots)
            visit(r, VXMat4::Identity());
    }

    bool GLTFModel::processMeshes(const json& root, const std::vector<uint8_t>& binData) {
        if (!root.contains("meshes")) {
            SetLastError(VXError("glTF has no meshes"));
            return false;
        }

        std::vector<VXMat4> meshWorldTransforms;
        computeNodeWorldTransforms(root, meshWorldTransforms);

        const auto& meshesJson = root["meshes"];
        for (size_t meshIdx = 0; meshIdx < meshesJson.size(); meshIdx++) {
            const auto& meshNode = meshesJson[meshIdx];
            std::string meshName = meshNode.value("name", "mesh");

            for (const auto& primitive : meshNode["primitives"]) {
                MeshData mesh;
                mesh.name = meshName;
                mesh.transform = meshIdx < meshWorldTransforms.size()
                    ? meshWorldTransforms[meshIdx] : VXMat4::Identity();

                if (primitive.contains("material"))
                    mesh.material = resolveMaterial(root, primitive["material"].get<int>(), binData);

                const auto& attrs = primitive["attributes"];

                std::vector<float> positions, normals, uvs;
                std::vector<uint32_t> indices;

                if (attrs.contains("POSITION"))
                    extractAccessor(root, attrs["POSITION"].get<int>(), binData, positions, indices, false);

                if (attrs.contains("NORMAL"))
                    extractAccessor(root, attrs["NORMAL"].get<int>(), binData, normals, indices, false);

                if (attrs.contains("TEXCOORD_0"))
                    extractAccessor(root, attrs["TEXCOORD_0"].get<int>(), binData, uvs, indices, false);

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
                        v.normal[0] = normals[i * 3 + 0];
                        v.normal[1] = normals[i * 3 + 1];
                        v.normal[2] = normals[i * 3 + 2];
                    } else {
                        v.normal[0] = v.normal[1] = v.normal[2] = 0.0f;
                    }

                    if (!uvs.empty() && (i * 2 + 1) < (int)uvs.size()) {
                        // glTF UVs are already top-left-origin (same convention Vulkan's
                        // sampler expects), unlike OBJ's bottom-up vt -- no flip needed here.
                        v.uv[0] = uvs[i * 2 + 0];
                        v.uv[1] = uvs[i * 2 + 1];
                    } else {
                        v.uv[0] = v.uv[1] = 0.0f;
                    }

                    // White when textured (so albedo*color passes the texture through
                    // unmodified), or the material's flat baseColor/diffuseFactor when this
                    // mesh has no texture at all (chrome, glass, solid-color trim etc.) --
                    // there's no per-material color uniform yet, so it's baked per-vertex.
                    // Alpha comes along the same way (e.g. this model's windshield glass is
                    // encoded as diffuseFactor alpha 0.15 with no texture at all).
                    v.color[0] = mesh.material.baseColor[0];
                    v.color[1] = mesh.material.baseColor[1];
                    v.color[2] = mesh.material.baseColor[2];
                    v.color[3] = mesh.material.baseColor[3];

                    mesh.vertices.push_back(v);
                }

                // glTF mandates counter-clockwise winding for front-facing triangles, but
                // VXRenderer's pipelines are built with frontFace = VK_FRONT_FACE_CLOCKWISE.
                // Left as-is, CULL_BACK keeps each triangle's actual back face and discards
                // the front -- invisible on a closed mesh (you just see the opposite side's
                // winding instead) but exposes open/thin-shell geometry (tire sidewalls etc.)
                // from the wrong side. Swap each triangle's last two indices to match VXGE's
                // convention.
                for (size_t t = 0; t + 2 < indices.size(); t += 3)
                    std::swap(indices[t + 1], indices[t + 2]);

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
                std::string uri = buf["uri"].get<std::string>();
                std::vector<uint8_t> embedded = tryDecodeDataUri(uri);
                if (!embedded.empty()) {
                    binData = std::move(embedded);
                } else {
                    fs::path binPath = path.parent_path() / uri;
                    std::ifstream binFile(binPath, std::ios::binary);
                    if (binFile.is_open())
                        binData = std::vector<uint8_t>((std::istreambuf_iterator<char>(binFile)),
                                                       std::istreambuf_iterator<char>());
                }
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
            else if (chunkType == 0x004E4942) binData = std::move(chunkData);
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

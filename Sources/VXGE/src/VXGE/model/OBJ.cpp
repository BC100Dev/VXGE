#include <VXGE/Model/OBJ.hpp>
#include <VXGE/VXError.hpp>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>

namespace VX {
    std::vector<OBJModel::MeshData>& OBJModel::GetMeshes() {
        return m_meshes;
    }

    bool OBJModel::IsLoaded() const {
        return m_loaded;
    }

    bool OBJModel::Load(const fs::path& path) {
        m_meshes.clear();
        m_loaded = false;
        return parse(path);
    }

    bool OBJModel::parse(const fs::path& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            SetLastError(VXError("Failed to open OBJ file: " + path.string()));
            return false;
        }

        std::vector<float> rawPositions;
        std::vector<float> rawNormals;
        std::vector<float> rawTexcoords;

        MeshData currentMesh;
        bool hasMesh = false;

        std::unordered_map<std::string, uint32_t> indexMap;

        auto flushMesh = [&]() {
            if (!currentMesh.vertices.empty()) {
                m_meshes.push_back(currentMesh);
                currentMesh = MeshData{};
                indexMap.clear();
            }
        };

        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;

            std::istringstream ss(line);
            std::string token;
            ss >> token;

            if (token == "o" || token == "g") {
                flushMesh();
                ss >> currentMesh.name;
                hasMesh = true;
            } else if (token == "v") {
                float x, y, z;
                ss >> x >> y >> z;
                rawPositions.push_back(x);
                rawPositions.push_back(y);
                rawPositions.push_back(z);
            } else if (token == "vn") {
                float x, y, z;
                ss >> x >> y >> z;
                rawNormals.push_back(x);
                rawNormals.push_back(y);
                rawNormals.push_back(z);
            } else if (token == "vt") {
                float u, v;
                ss >> u >> v;
                rawTexcoords.push_back(u);
                rawTexcoords.push_back(v);
            } else if (token == "f") {
                std::vector<std::string> faceTokens;
                std::string ft;
                while (ss >> ft) faceTokens.push_back(ft);

                for (size_t i = 1; i + 1 < faceTokens.size(); i++) {
                    std::string triTokens[3] = {faceTokens[0], faceTokens[i], faceTokens[i + 1]};
                    for (const auto& fv : triTokens) {
                        auto it = indexMap.find(fv);
                        if (it != indexMap.end()) {
                            currentMesh.indices.push_back(it->second);
                            continue;
                        }

                        int vi = 0, vti = 0, vni = 0;
                        bool hasVti = false;

                        std::istringstream fvss(fv);
                        std::string part;
                        int partIdx = 0;

                        while (std::getline(fvss, part, '/')) {
                            if (!part.empty()) {
                                int val = std::stoi(part);
                                if (partIdx == 0)      vi  = val;
                                else if (partIdx == 1) { vti = val; hasVti = true; }
                                else if (partIdx == 2) vni = val;
                            }
                            partIdx++;
                        }

                        vi  = vi  > 0 ? vi  - 1 : (int)(rawPositions.size() / 3) + vi;
                        vni = vni > 0 ? vni - 1 : (int)(rawNormals.size()    / 3) + vni;
                        vti = hasVti ? (vti > 0 ? vti - 1 : (int)(rawTexcoords.size() / 2) + vti) : -1;

                        VXVertex vert{};
                        if (vi >= 0 && (vi * 3 + 2) < (int)rawPositions.size()) {
                            vert.position[0] = rawPositions[vi * 3 + 0];
                            vert.position[1] = rawPositions[vi * 3 + 1];
                            vert.position[2] = rawPositions[vi * 3 + 2];
                        }

                        if (vti >= 0 && (vti * 2 + 1) < (int)rawTexcoords.size()) {
                            vert.uv[0] = rawTexcoords[vti * 2 + 0];
                            // OBJ's vt v-coordinate is bottom-up; Vulkan/VXGE's sampler expects top-down.
                            vert.uv[1] = 1.0f - rawTexcoords[vti * 2 + 1];
                        } else {
                            vert.uv[0] = 0.0f;
                            vert.uv[1] = 0.0f;
                        }

                        if (vni >= 0 && (vni * 3 + 2) < (int)rawNormals.size()) {
                            vert.normal[0] = rawNormals[vni * 3 + 0];
                            vert.normal[1] = rawNormals[vni * 3 + 1];
                            vert.normal[2] = rawNormals[vni * 3 + 2];
                        } else {
                            vert.normal[0] = 0.0f;
                            vert.normal[1] = 0.0f;
                            vert.normal[2] = 0.0f;
                        }

                        // vert.color used to carry the normal (remapped to 0..1) as a debug
                        // visualization before VXVertex had a real normal field. Now that it
                        // does, color is a plain white multiplier so lit.frag's albedo*color
                        // passes the texture/lighting result through unmodified.
                        vert.color[0] = 1.0f;
                        vert.color[1] = 1.0f;
                        vert.color[2] = 1.0f;

                        uint32_t newIndex = static_cast<uint32_t>(currentMesh.vertices.size());
                        currentMesh.vertices.push_back(vert);
                        currentMesh.indices.push_back(newIndex);
                        indexMap[fv] = newIndex;
                    }
                }
            }
        }

        flushMesh();

        if (m_meshes.empty() && !rawPositions.empty()) {
            MeshData singleMesh = currentMesh;
            singleMesh.name = "default";
            m_meshes.push_back(singleMesh);
        }

        m_loaded = !m_meshes.empty();
        if (!m_loaded)
            SetLastError(VXError("OBJ file contained no geometry: " + path.string()));

        return m_loaded;
    }
}
#include <iostream>
#include <filesystem>
#include <random>
#include <array>
#include <cstring>

#include <VXGE/VX.hpp>
#include <VXGE/Model/GLTF.hpp>

#ifdef __linux__
#include <linux/limits.h>
#include <unistd.h>
#elif defined(_WIN32)
#include <windows.h>
#endif

namespace fs = std::filesystem;

fs::path ExecutableFile() {
#ifdef _WIN32
    char buffer[MAX_PATH];
    GetModuleFileName(nullptr, buffer, MAX_PATH);
    fs::path exePath = std::string(buffer);
    return exePath.string();
#else
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    fs::path exePath = std::string(result, (count > 0) ? count : 0);
    return exePath.string();
#endif
}

fs::path ExecutableDirectory() {
    return ExecutableFile().parent_path();
}

int main() {
    VX::VXEngine ctx("Test Game");
    if (!ctx.Initialize()) {
        std::cerr << "Engine init failed: " << VX::GetLastError().what() << std::endl;
        return 1;
    }

    std::vector<VX::VXGraphics> graphicCards = ctx.EnumerateGraphicsCards();
    std::vector<VX::VXMonitor> monitors = ctx.EnumerateMonitors();

    if (graphicCards.empty() || monitors.empty()) {
        std::cerr << "No GPUs or monitors found" << std::endl;
        return 1;
    }

    VX::VXWindow wnd = ctx.CreateWindow("VXGE Demo", 1280u, 720u, {VX::VXFlags::WINDOW_FULLSCREEN});
    if (!wnd.Populate()) {
        std::cerr << "Window creation failed: " << VX::GetLastError().what() << std::endl;
        return 1;
    }

    VX::VXRenderer renderer = ctx.CreateRenderer(wnd, graphicCards[0], {VX::VXFlags::RENDERER_USE_LIGHTS});
    renderer.SetVSync(true);
    if (!renderer.Populate()) {
        std::cerr << "Renderer creation failed: " << VX::GetLastError().what() << std::endl;
        return 1;
    }

    std::vector<VX::VXVertex> vertices = {
        {{-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
        {{0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
        {{0.0f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.5f, 0.0f}},

        {{0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
        {{0.5f, -0.5f, 0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
        {{0.0f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.5f, 0.0f}},

        {{0.5f, -0.5f, 0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
        {{-0.5f, -0.5f, 0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
        {{0.0f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.5f, 0.0f}},

        {{-0.5f, -0.5f, 0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
        {{-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
        {{0.0f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.5f, 0.0f}},

        {{-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
        {{0.5f, -0.5f, 0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
        {{0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},

        {{-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
        {{-0.5f, -0.5f, 0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
        {{0.5f, -0.5f, 0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
    };

    for (size_t i = 0; i + 2 < vertices.size(); i += 3) {
        VX::VXVec3 p0(vertices[i].position[0], vertices[i].position[1], vertices[i].position[2]);
        VX::VXVec3 p1(vertices[i + 1].position[0], vertices[i + 1].position[1], vertices[i + 1].position[2]);
        VX::VXVec3 p2(vertices[i + 2].position[0], vertices[i + 2].position[1], vertices[i + 2].position[2]);

        VX::VXVec3 n = (p1 - p0).cross(p2 - p0).normalized();

        for (size_t v = i; v < i + 3; v++) {
            vertices[v].normal[0] = n.x;
            vertices[v].normal[1] = n.y;
            vertices[v].normal[2] = n.z;
        }
    }

    std::vector<uint32_t> indices;
    for (uint32_t i = 0; i < static_cast<uint32_t>(vertices.size()); i++)
        indices.push_back(i);

    VX::VXMesh pyramidMesh = renderer.CreateMesh(vertices, indices);

    fs::path litVert = ExecutableDirectory() / "shaders" / "lit.vert.spv";
    fs::path litFrag = ExecutableDirectory() / "shaders" / "lit.frag.spv";

    VX::VXMaterial pyramidMaterial = renderer.CreateMaterial(litVert, litFrag, {VX::VXFlags::CULL_NONE});

    VX::VXLight sunLight;
    sunLight.SetDirection(-0.8f, -0.5f, 0.3f);
    sunLight.SetColor(1.0f, 1.0f, 1.0f);
    sunLight.SetIntensity(1.5f);

    VX::VXTexture checkerTexture = renderer.CreateTexture(ExecutableDirectory() / "textures" / "checker.png");
    if (VX::GetLastError().Empty()) {
        pyramidMaterial.SetAlbedo(&checkerTexture);
        renderer.BindMaterialTextures(pyramidMaterial);
    } else {
        std::cerr << "Failed to load checker texture: " << VX::GetLastError().what() << std::endl;
    }

    VX::GLTFModel carModel;
    std::vector<VX::VXMesh> carMeshes;
    std::vector<VX::VXMaterial> carMaterials;
    std::vector<VX::VXTexture> carTextures;
    std::vector<std::array<float, 16>> carMeshTransforms;

    fs::path alfaDir = ExecutableDirectory() / "testmodels" / "alfa_romeo_stradale_1967";

    int meshesWithTextureFlag = 0;
    int texturesLoaded = 0;
    int texturesFailed = 0;

    if (!carModel.Load(alfaDir / "scene.gltf")) {
        std::cerr << "Failed to load car model: " << VX::GetLastError().what() << std::endl;
    } else {
        carMaterials.reserve(carModel.GetMeshes().size());
        carTextures.reserve(carModel.GetMeshes().size());

        for (auto& meshData : carModel.GetMeshes()) {
            if (meshData.name.find("VRayMtl1") != std::string::npos)
                continue;

            VX::VXMesh mesh = renderer.CreateMesh(meshData.vertices, meshData.indices);

            bool isGlass = meshData.name.find("mafina2hdri") != std::string::npos;
            std::vector<VX::VXFlags> matFlags = {VX::VXFlags::CULL_BACK};
            if (isGlass)
                matFlags.push_back(VX::VXFlags::MESH_TRANSPARENT);

            VX::VXMaterial material = renderer.CreateMaterial(litVert, litFrag, matFlags);

            if (meshData.material.hasTexture) {
                meshesWithTextureFlag++;

                VX::VXTexture tex = meshData.material.bytes.empty()
                                        ? renderer.CreateTexture(meshData.material.path)
                                        : renderer.CreateTextureFromMemory(meshData.material.bytes);

                if (tex.GetImage() != VK_NULL_HANDLE) {
                    texturesLoaded++;
                    carTextures.push_back(tex);
                    material.SetAlbedo(&carTextures.back());
                } else {
                    texturesFailed++;
                    std::cerr << "Failed to load texture for mesh '" << meshData.name << "' (path: "
                        << meshData.material.path.string() << ", bytes: " << meshData.material.bytes.size()
                        << "): " << VX::GetLastError().what() << std::endl;
                }
            }

            renderer.BindMaterialTextures(material);

            carMaterials.push_back(material);
            carMeshes.push_back(mesh);

            std::array<float, 16> transform{};
            std::memcpy(transform.data(), meshData.transform.Data(), sizeof(float) * 16);
            carMeshTransforms.push_back(transform);
        }
    }

    std::cout << "Car meshes with a texture flag: " << meshesWithTextureFlag
        << ", loaded OK: " << texturesLoaded << ", failed: " << texturesFailed << std::endl;

    float pyramidTransform[16] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };

    VX::VXMat4 carPlacement = VX::VXMat4::Translation({3.0f, 0.0f, 0.0f});
    VX::VXMat4 carScale{};
    std::memset(carScale.m, 0, sizeof(carScale.m));
    carScale.m[0] = carScale.m[5] = carScale.m[10] = 7.5f;
    carScale.m[15] = 1.0f; // homogeneous W: must stay 1, never the scale factor
    carPlacement = carPlacement * carScale;

    float aspect = (float)wnd.Width() / (float)wnd.Height();
    VX::VXCamera camera(75.0f, aspect, 0.1f, 100.0f);
    camera.SetPosition({0.0f, 0.0f, -5.0f});

    wnd.SetOnCloseRequest([&] {
        wnd.UnlockMouse();
        return true;
    });
    wnd.SetOnEvent([&](VX::VXEvent& event) {
        if (event.type == VX::VXEventType::MouseMove && wnd.IsMouseLocked())
            camera.Rotate(-event.mouse.deltaX * 0.2f, event.mouse.deltaY * 0.2f);

        if (event.type == VX::VXEventType::MouseDown && event.mouse.button == 1)
            wnd.LockMouse();

        if (event.type == VX::VXEventType::KeyDown && event.key.keyCode == VX::VXKey::Escape)
            wnd.FireCloseEvent();

        if (event.type == VX::VXEventType::KeyDown && (event.key.keyCode == VX::VXKey::KpEnter || event.key.keyCode ==
            VX::VXKey::Enter))
            camera.SetPosition({0.0f, 0.0f, -5.0f});
    });
    wnd.LockMouse();
    wnd.SetCloseKey(VX::VXKey::Q);

    ctx.LockWindowContext();
    ctx.ShowWindow(wnd);
    renderer.SetTargetFPS(120);

    float fpsTimer = 0.0f;
    int fpsCounter = 0;

    while (!wnd.IsClosing()) {
        bool fpsFirstTime = ctx.TickTime();
        wnd.PollEvents();

        VX::VXVec3 forward = camera.GetForward();
        VX::VXVec3 right = camera.GetRight();

        float delta = ctx.DeltaTime();
        float speed = 3.0f * delta;

        if (wnd.IsKeyHeld(VX::VXKey::LeftShift) || wnd.IsKeyHeld(VX::VXKey::RightShift))
            speed *= 8;
        if (wnd.IsKeyHeld(VX::VXKey::LeftAlt) || wnd.IsKeyHeld(VX::VXKey::RightAlt))
            speed /= 8;
        if (wnd.IsKeyHeld(VX::VXKey::W))
            camera.Move(forward * speed);
        if (wnd.IsKeyHeld(VX::VXKey::S))
            camera.Move(forward * -speed);
        if (wnd.IsKeyHeld(VX::VXKey::A))
            camera.Move(right * -speed);
        if (wnd.IsKeyHeld(VX::VXKey::D))
            camera.Move(right * speed);
        if (wnd.IsKeyHeld(VX::VXKey::Space) || wnd.IsKeyHeld(VX::VXKey::E))
            camera.Move({0.0f, speed, 0.0f});
        if (wnd.IsKeyHeld(VX::VXKey::LeftCtrl) || wnd.IsKeyHeld(VX::VXKey::RightCtrl))
            camera.Move({0.0f, -speed, 0.0f});

        if (fpsFirstTime) {
            fpsTimer = 0.0f;
            fpsCounter = 0;
        }

        fpsTimer += delta;

        if (fpsTimer >= 1.0f) {
            float fps = static_cast<float>(fpsCounter) / fpsTimer;
            std::cout << "FPS: " << fps << std::endl;

            fpsTimer = 0.0f;
            fpsCounter = 0;
        }

        renderer.SetCamera(camera);
        renderer.SubmitLight(sunLight);

        renderer.Submit(pyramidMesh, pyramidMaterial, pyramidTransform);

        for (size_t i = 0; i < carMeshes.size(); i++) {
            VX::VXMat4 meshTransform{};
            std::memcpy(meshTransform.m, carMeshTransforms[i].data(), sizeof(float) * 16);
            VX::VXMat4 finalTransform = carPlacement * meshTransform;
            renderer.Submit(carMeshes[i], carMaterials[i], finalTransform.Data());
        }

        renderer.Render();
        fpsCounter++;
    }

    pyramidMesh.Destroy(graphicCards[0].Device());
    for (auto& carMesh : carMeshes)
        carMesh.Destroy(graphicCards[0].Device());

    checkerTexture.Destroy(graphicCards[0].Device());
    for (auto& tex : carTextures)
        tex.Destroy(graphicCards[0].Device());

    pyramidMaterial.Destroy(graphicCards[0].Device());
    for (auto& mat : carMaterials)
        mat.Destroy(graphicCards[0].Device());

    ctx.DestroyRenderer(renderer);
    ctx.DestroyWindow(wnd);
    ctx.DestroyGraphics(graphicCards);

    ctx.Shutdown();

    return 0;
}

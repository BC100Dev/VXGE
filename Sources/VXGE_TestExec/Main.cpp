#include <iostream>
#include <filesystem>
#include <random>

#include <VXGE/VX.hpp>
#include <VXGE/Model/OBJ.hpp>

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

float randomFloat(float min, float max) {
    static std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution dist(min, max);
    return dist(rng);
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

    VX::VXWindow wnd = ctx.CreateWindow("VXGE Demo", monitors[0], VX::VXFlags::WINDOW_WINDOWED);
    if (!wnd.Populate()) {
        std::cerr << "Window creation failed: " << VX::GetLastError().what() << std::endl;
        return 1;
    }

    VX::VXRenderer renderer = ctx.CreateRenderer(wnd, graphicCards[0], VX::VXFlags::RENDERER_3D);
    renderer.SetVSync(true);
    if (!renderer.Populate()) {
        std::cerr << "Renderer creation failed: " << VX::GetLastError().what() << std::endl;
        return 1;
    }

    std::vector<VX::VXVertex> vertices = {
        // apex
        {{0.0f, 0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}}, // 0 - red

        // base
        {{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}}, // 1 - green
        {{0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}}, // 2 - blue
        {{0.5f, -0.5f, 0.5f}, {1.0f, 1.0f, 0.0f}}, // 3 - yellow
        {{-0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 1.0f}}, // 4 - magenta
    };

    std::vector<uint32_t> indices = {
        // 4 triangular side faces
        0, 1, 2,
        0, 2, 3,
        0, 3, 4,
        0, 4, 1,

        // square base split into 2 triangles
        1, 3, 2,
        1, 4, 3,
    };
    VX::VXMesh pyramidMesh = renderer.CreateMesh(vertices, indices);

    fs::path vert = ExecutableDirectory() / "shaders" / "triangle.vert.spv";
    fs::path frag = ExecutableDirectory() / "shaders" / "triangle.frag.spv";

    VX::VXMaterial pyramidMaterial = renderer.CreateMaterial(vert, frag, VX::VXFlags::CULL_NONE);
    VX::VXMaterial carMaterial = renderer.CreateMaterial(vert, frag, VX::VXFlags::CULL_BACK);

    // test OBJ
    VX::OBJModel carModel;
    std::vector<VX::VXMesh> carMeshes;

    if (!carModel.Load(ExecutableDirectory() / "testmodels" / "Porsche_911_GT2.obj")) {
        std::cerr << "Failed to load car model: " << VX::GetLastError().what() << std::endl;
    } else {
        for (auto& meshData : carModel.GetMeshes()) {
            for (auto& vertex : meshData.vertices) {
                // antimatter looking ass car
                vertex.color[0] = 1.0f;
                vertex.color[1] = 1.0f;
                vertex.color[2] = 1.0f;
            }

            carMeshes.push_back(renderer.CreateMesh(meshData.vertices, meshData.indices));
        }
    }

    float pyramidTransform[16] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };

    float carTransform[16] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        3, 0, 0, 1
    };

    float aspect = (float)monitors[0].Width() / (float)monitors[0].Height();
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
    // to remove the close key: wnd.ClearCloseKey();

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
        if (wnd.IsKeyHeld(VX::VXKey::W))
            camera.Move(forward * speed);
        if (wnd.IsKeyHeld(VX::VXKey::S))
            camera.Move(forward * -speed);
        if (wnd.IsKeyHeld(VX::VXKey::A))
            camera.Move(right * -speed);
        if (wnd.IsKeyHeld(VX::VXKey::D))
            camera.Move(right * speed);
        if (wnd.IsKeyHeld(VX::VXKey::Space))
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
        renderer.Submit(pyramidMesh, pyramidMaterial, pyramidTransform);

        for (auto& carMesh : carMeshes)
            renderer.Submit(carMesh, carMaterial, carTransform);

        renderer.Render();
        fpsCounter++;
    }

    pyramidMesh.Destroy(graphicCards[0].Device());
    for (auto& carMesh : carMeshes)
        carMesh.Destroy(graphicCards[0].Device());

    pyramidMaterial.Destroy(graphicCards[0].Device());
    carMaterial.Destroy(graphicCards[0].Device());

    ctx.DestroyRenderer(renderer);
    ctx.DestroyWindow(wnd);
    ctx.DestroyGraphics(graphicCards);

    ctx.Shutdown();

    return 0;
}

#include <iostream>
#include <filesystem>

#include <VXGE/VX.hpp>

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
    VX::VXEngine ctx("Base Game");
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
    if (!renderer.Populate()) {
        std::cerr << "Renderer creation failed: " << VX::GetLastError().what() << std::endl;
        return 1;
    }

    std::vector<VX::VXVertex> vertices = {
        {{0.0f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, 0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
        {{-0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}},
    };

    std::vector<uint32_t> indices = {0, 1, 2};
    VX::VXMesh mesh = renderer.CreateMesh(vertices, indices);
    VX::VXMaterial material = renderer.CreateMaterial(
        ExecutableDirectory().string() + "/shaders/triangle.vert.spv",
        ExecutableDirectory().string() + "/shaders/triangle.frag.spv");

    float identity[16] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };

    float aspect = (float)monitors[0].Width() / (float)monitors[0].Height();
    VX::VXCamera camera(75.0f, aspect, 0.1f, 100.0f);
    camera.SetPosition({0.0f, 0.0f, -2.0f});

    wnd.SetOnEvent([&](VX::VXEvent& event) {
        if (event.type == VX::VXEventType::MouseMove && wnd.IsMouseLocked())
            camera.Rotate(-event.mouse.deltaX * 0.1f, event.mouse.deltaY * 0.1f);

        if (event.type == VX::VXEventType::MouseDown && event.mouse.button == 1)
            wnd.LockMouse();
    });
    wnd.SetOnCloseRequest([&] {
        wnd.UnlockMouse();
        return true;
    });
    wnd.LockMouse();
    wnd.SetCloseKey(VX::VXKey::Escape);
    // to remove the close key: wnd.ClearCloseKey();

    ctx.LockWindowContext();
    ctx.ShowWindow(wnd);

    while (!wnd.IsClosing()) {
        ctx.TickTime();

        wnd.PollEvents();

        VX::VXVec3 forward = camera.GetForward();
        VX::VXVec3 right = camera.GetRight();
        float speed = 3.0f * ctx.DeltaTime();

        if (wnd.IsKeyHeld(VX::VXKey::W))
            camera.Move(forward * speed);
        if (wnd.IsKeyHeld(VX::VXKey::S))
            camera.Move(forward * -speed);
        if (wnd.IsKeyHeld(VX::VXKey::A))
            camera.Move(right * -speed);
        if (wnd.IsKeyHeld(VX::VXKey::D))
            camera.Move(right * speed);
        if (wnd.IsKeyHeld(VX::VXKey::LeftCtrl) || wnd.IsKeyHeld(VX::VXKey::RightCtrl))
            camera.Move({0.0f, -speed, 0.0f});
        if (wnd.IsKeyHeld(VX::VXKey::LeftShift) || wnd.IsKeyHeld(VX::VXKey::RightShift))
            camera.Move({0.0f, speed, 0.0f});

        renderer.SetCamera(camera);
        renderer.Submit(mesh, material, identity);
        renderer.Render();
    }

    mesh.Destroy(graphicCards[0].Device());
    material.Destroy(graphicCards[0].Device());

    ctx.DestroyRenderer(renderer);
    ctx.DestroyWindow(wnd);

    graphicCards.clear(); // necessary, otherwise vkDestroyDevice has Invalid device error
    ctx.Shutdown();

    return 0;
}

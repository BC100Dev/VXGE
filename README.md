# VXGE — Vulkan-powered Game Engine
VXGE is a C++ game engine library built on top of SDL3 and Vulkan, designed to
abstract the Vulkan rendering pipeline away from game code so developers can focus on
building games rather than managing GPU infrastructure.

## Usage

```cpp
#include <VXGE/VX.hpp>
#include <iostream>

int main() {
    VX::VXEngine ctx;

    if (!ctx.Initialize()) {
        std::cerr << "Unable to initialize engine: " << VX::GetLastError().What() << std::endl;
        return 1;
    }

    std::vector<VX::VXGraphics> gpus = ctx.EnumerateGraphicsCards();
    std::vector<VX::VXMonitor> monitors = ctx.EnumerateMonitors();

    VX::VXWindow wnd = ctx.CreateWindow("My Game", monitors[0], VX::VXFlags::WINDOW_FULLSCREEN);
    if (!wnd.Populate()) {
        std::cerr << "Unable to create window: " << VX::GetLastError().What() << std::endl;
        return 1;
    }

    VX::VXRenderer renderer = ctx.CreateRenderer(wnd, gpus[0], VX::VXFlags::VX_FLAGS_3D);
    if (!renderer.Populate()) {
        std::cerr << "Unable to create renderer: " << VX::GetLastError().What() << std::endl;
        return 1;
    }

    ctx.LockWindowContext();
    ctx.ShowWindow(wnd);

    while (!wnd.IsClosing()) {
        wnd.PollEvents();
        renderer.Render();
    }

    ctx.DestroyRenderer(renderer);
    ctx.DestroyWindow(wnd);
    ctx.Shutdown();

    return 0;
}
```

## License

See LICENSE for details.
---
name: vxge
description: "Reference guide for writing correct C++ game code using VXGE (Vulkan-powered Game Engine). Use when working on any project that includes VXGE, when writing code that uses VX:: namespace classes, when setting up a game loop, renderer, window, camera, input, or model loading with VXGE. Covers all public API methods, correct method names, initialization order, destruction order, shader requirements, and common mistakes."
---

# VXGE — Vulkan-powered Game Engine — AI Skill Reference

## Purpose

This document exists to give AI models enough context to write correct, idiomatic VXGE code without hallucinating Vulkan or SDL calls into game code. VXGE fully abstracts both SDL3 and Vulkan. Game code never touches `VkDevice`, `SDL_Window`, `SDL_Event`, raw scancodes, or any other low-level handle directly. If you find yourself writing `vkCreateBuffer` or `SDL_PollEvent` in game code, you are doing it wrong.

---

## Namespace

Everything lives under `VX::`. There are no exceptions.

---

## Include Strategy

Single umbrella header for full access:

    #include <VXGE/VX.hpp>

`VX.hpp` now includes `VXAudio.hpp` and `VXOverlay.hpp` — both are part of the umbrella header.

Individual headers if needed:

    #include <VXGE/VXEngine.hpp>
    #include <VXGE/VXGraphics.hpp>
    #include <VXGE/VXMonitor.hpp>
    #include <VXGE/VXWindow.hpp>
    #include <VXGE/VXRenderer.hpp>
    #include <VXGE/VXMesh.hpp>
    #include <VXGE/VXMaterial.hpp>
    #include <VXGE/VXCamera.hpp>
    #include <VXGE/VXMath.hpp>
    #include <VXGE/VXEvent.hpp>
    #include <VXGE/VXKey.hpp>
    #include <VXGE/VXFlags.hpp>
    #include <VXGE/VXError.hpp>
    #include <VXGE/VXHaptic.hpp>
    #include <VXGE/VXAudio.hpp>
    #include <VXGE/VXOverlay.hpp>

Model loaders are NOT in VX.hpp — include them separately:

    #include <VXGE/Model/OBJ.hpp>
    #include <VXGE/Model/GLTF.hpp>

---

## Initialization and Shutdown Order

This order is mandatory. Deviating from it causes Vulkan validation errors or crashes.

    VX::VXEngine ctx("App Name");
    ctx.Initialize();

    std::vector<VX::VXGraphics> gpus     = ctx.EnumerateGraphicsCards();
    std::vector<VX::VXMonitor>  monitors = ctx.EnumerateMonitors();

    VX::VXWindow wnd = ctx.CreateWindow("Title", monitors[0], VX::VXFlags::WINDOW_WINDOWED);
    wnd.Populate();

    VX::VXRenderer renderer = ctx.CreateRenderer(wnd, gpus[0], VX::VXFlags::RENDERER_3D);
    renderer.Populate();

    // Optional overlay (ImGui-backed, must be created after renderer.Populate())
    VX::VXOverlay overlay = ctx.CreateOverlay(wnd, gpus[0], renderer);
    overlay.Populate();

    // ... game loop ...

    overlay.Destroy();
    mesh.Destroy(gpus[0].Device());
    material.Destroy(gpus[0].Device());
    ctx.DestroyRenderer(renderer);
    ctx.DestroyWindow(wnd);
    ctx.DestroyGraphics(gpus);   // preferred over gpus.clear() — calls Destroy() on each, then clears
    ctx.Shutdown();

Calling `ctx.Shutdown()` before destroying graphics will cause `vkDestroyDevice: Invalid device` because the Vulkan instance is destroyed before the logical devices. Use `ctx.DestroyGraphics(gpus)` instead of `gpus.clear()` — it properly calls `gpu.Destroy()` on each entry before clearing.

---

## VX::VXEngine

Central orchestrator. Always stack-allocated, never heap-allocated with `new`.

    VX::VXEngine ctx("App Name");

    bool  ctx.Initialize();                              // initializes SDL3 + Vulkan instance
    void  ctx.Shutdown();                                // destroys Vulkan instance + SDL
    bool  ctx.TickTime();                                // call once per frame at top of loop — returns true on first frame
    float ctx.DeltaTime();                               // seconds since last TickTime() call
    void  ctx.LockWindowContext();                       // restricts to one window/renderer
    void  ctx.ShowWindow(wnd);                           // makes window visible
    void  ctx.DestroyWindow(wnd);                        // destroys SDL window
    void  ctx.DestroyRenderer(renderer);                 // calls renderer.Destroy()
    void  ctx.DestroyGraphics(gpus);                     // calls Destroy() on each gpu, then clears the vector

    std::vector<VX::VXGraphics> ctx.EnumerateGraphicsCards();
    std::vector<VX::VXMonitor>  ctx.EnumerateMonitors();
    std::vector<VX::VXHaptic>   ctx.EnumerateHapticDevices();

    VX::VXWindow   ctx.CreateWindow(title, monitor, flags);
    VX::VXRenderer ctx.CreateRenderer(window, graphics, flags);
    VX::VXOverlay  ctx.CreateOverlay(window, graphics, renderer);   // must call after renderer.Populate()

TickTime() returns true on the very first call — use this to skip the first frame's garbage delta:

    while (!wnd.IsClosing()) {
        bool firstFrame = ctx.TickTime();
        if (firstFrame) continue;
        // ... rest of loop
    }

---

## VX::VXGraphics

Wraps a Vulkan physical and logical device. Returned by `EnumerateGraphicsCards()`.

    const std::string& gpu.DeviceName();
    VkPhysicalDevice   gpu.PhysicalDevice();
    VkDevice           gpu.Device();
    VkQueue            gpu.GraphicsQueue();
    VkQueue            gpu.PresentQueue();
    uint32_t           gpu.GraphicsFamily();
    uint32_t           gpu.PresentFamily();
    void               gpu.Destroy();                   // explicit cleanup — also called by DestroyGraphics()

VXGraphics is NOT copyable. It is move-only. Never copy a VXGraphics object. Storing them in a `std::vector` is fine because the vector uses move semantics internally, but you MUST call `ctx.DestroyGraphics(gpus)` before `ctx.Shutdown()`.

    // CORRECT
    std::vector<VX::VXGraphics> gpus = ctx.EnumerateGraphicsCards();
    ctx.DestroyGraphics(gpus);   // before ctx.Shutdown()
    ctx.Shutdown();

    // WRONG — never do this
    VX::VXGraphics copy = gpus[0]; // compile error, copy is deleted

---

## VX::VXMonitor

Wraps display information. Returned by `EnumerateMonitors()`. Pure data, no Vulkan handles, no cleanup needed.

    int                monitor.Width();
    int                monitor.Height();
    float              monitor.RefreshRate();
    const std::string& monitor.Name();
    SDL_DisplayID      monitor.DisplayID();

---

## VX::VXWindow

Wraps SDL3 window and Vulkan surface creation.

    VX::VXWindow wnd = ctx.CreateWindow("Title", monitors[0], VX::VXFlags::WINDOW_WINDOWED);
    bool ok = wnd.Populate();   // finalizes SDL3 window creation — must be called before use

    bool               wnd.IsClosing();          // returns true when window close is requested
    void               wnd.PollEvents();         // must be called every frame — updates input state
    const std::string& wnd.Title();              // window title
    uint32_t           wnd.Width();              // window width in pixels
    uint32_t           wnd.Height();             // window height in pixels
    SDL_Window*        wnd.SDLWindow();          // raw SDL handle — only use internally in VXGE
    bool               wnd.IsKeyHeld(VXKey);     // returns true if key is currently held down
    void               wnd.LockMouse();          // locks cursor, enables relative mouse mode
    void               wnd.UnlockMouse();        // unlocks cursor
    bool               wnd.IsMouseLocked();      // returns true if mouse is locked
    void               wnd.Close();              // manually triggers window close
    void               wnd.FireCloseEvent();     // fires close request through callback chain

    void wnd.SetCloseKey(VX::VXKey key);         // opt-in: sets a key that closes the window
    void wnd.ClearCloseKey();                    // removes the close key

    void wnd.SetOnCloseRequest(std::function<bool()> callback);
    // no callback = always close; callback returns true to allow close, false to block it

    void wnd.SetOnEvent(std::function<void(VX::VXEvent&)> callback);
    // replaces the single event callback — fired for every event

    void wnd.AddOnEvent(std::function<void(VX::VXEvent&)> callback);
    // adds an additional event callback on top of any existing one
    // VXOverlay uses this internally — do not call SetOnEvent after CreateOverlay or you will stomp the overlay's callback

VXWindow is NOT copyable. Move-only.

---

## VX::VXRenderer

Wraps swapchain, render pass, framebuffers, command buffers, sync objects, descriptor sets, UBO, and depth buffer.

    VX::VXRenderer renderer = ctx.CreateRenderer(wnd, gpus[0], VX::VXFlags::RENDERER_3D);
    bool ok = renderer.Populate();   // must be called before use

    VX::VXMesh     renderer.CreateMesh(vertices, indices);
    VX::VXMaterial renderer.CreateMaterial(vertSpvPath, fragSpvPath, cullMode = VXFlags::CULL_BACK);

    void renderer.Submit(mesh, material, transform);   // queues a draw call for this frame
    void renderer.SetCamera(camera);                   // uploads VP matrix to UBO
    void renderer.Render();                            // submits frame, presents, clears queue
    void renderer.Destroy();                           // explicit cleanup — also called by destructor

    void renderer.SetTargetFPS(int fps);               // enables FPS cap
    void renderer.ClearTargetFPS();                    // removes FPS cap
    void renderer.SetVSync(bool enabled);              // toggles vsync — can be called at any time
    void renderer.SetClearColor(float r, float g, float b, float a);   // sets background clear color (default 0.1, 0.1, 0.1, 1.0)

    // Internal accessors — used by VXOverlay, do not call from game code
    VkInstance       renderer.GetInstance();
    VkRenderPass     renderer.GetRenderPass();
    VkCommandBuffer  renderer.GetCurrentCommandBuffer();

Shaders must be pre-compiled SPIR-V `.spv` files. VXGE does not compile GLSL at runtime. Compile with:

    glslc shader.vert -o shader.vert.spv
    glslc shader.frag -o shader.frag.spv

The vertex shader MUST have this layout:

    layout(binding = 0) uniform UBO { mat4 mvp; } ubo;
    layout(push_constant) uniform PushConstants { mat4 model; } push;

    void main() {
        gl_Position = ubo.mvp * push.model * vec4(inPosition, 1.0);
    }

VXRenderer is NOT copyable. Move-only.

---

## VX::VXMesh

Holds GPU vertex and index buffers. Created via `renderer.CreateMesh()`.

    std::vector<VX::VXVertex> vertices = {
        {{ 0.0f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f }},
        {{ 0.5f,  0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f }},
        {{-0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }},
    };
    std::vector<uint32_t> indices = { 0, 1, 2 };

    VX::VXMesh mesh = renderer.CreateMesh(vertices, indices);
    mesh.Destroy(gpus[0].Device());   // must be called before DestroyRenderer

VXVertex layout:

    struct VXVertex {
        float position[3];   // location 0 in vertex shader
        float color[3];      // location 1 in vertex shader
    };

---

## VX::VXMaterial

Holds a compiled Vulkan graphics pipeline. Created via `renderer.CreateMaterial()`.

    VX::VXMaterial material = renderer.CreateMaterial(
        fs::path("/path/to/shader.vert.spv"),
        fs::path("/path/to/shader.frag.spv"),
        VX::VXFlags::CULL_BACK   // optional, defaults to CULL_BACK
    );
    material.Destroy(gpus[0].Device());   // must be called before DestroyRenderer

---

## VX::VXCamera

First-person camera with yaw/pitch rotation and perspective projection. Hand-rolled math, no GLM.

    VX::VXCamera camera(fovDegrees, aspectRatio, nearPlane, farPlane);

    camera.SetPosition({ 0.0f, 0.0f, -2.0f });
    camera.SetRotation(yaw, pitch);       // degrees
    camera.Move(delta);                   // VXVec3 offset
    camera.Rotate(dyaw, dpitch);          // degrees, pitch clamped to -89/+89

    VX::VXVec3 camera.GetForward();
    VX::VXVec3 camera.GetRight();
    VX::VXVec3 camera.GetPosition();
    float      camera.GetYaw();
    float      camera.GetPitch();
    VX::VXMat4 camera.GetView();
    VX::VXMat4 camera.GetProjection();
    VX::VXMat4 camera.GetVP();

    renderer.SetCamera(camera);           // uploads VP matrix — call every frame before Render()

Aspect ratio for the camera should be computed from the monitor:

    float aspect = (float)monitors[0].Width() / (float)monitors[0].Height();
    VX::VXCamera camera(75.0f, aspect, 0.1f, 100.0f);

---

## VX::VXMath — VXVec3 and VXMat4

    VX::VXVec3 v(x, y, z);
    v + other, v - other, v * scalar
    v += other, v -= other
    v.dot(other)
    v.cross(other)
    v.length()
    v.normalized()

    VX::VXMat4 m;
    VX::VXMat4::Identity()
    VX::VXMat4::Perspective(fovRad, aspect, near, far)
    VX::VXMat4::LookAt(eye, center, up)
    VX::VXMat4::Translation(vec3)
    m * other
    m.Data()    // const float* — 16 floats, column-major

Transform matrices passed to Submit() are column-major. Translation goes in the last row:

    float transform[16] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        x, y, z, 1   // translation here
    };

---

## VX::VXEvent and VX::VXEventType

Events are delivered via `wnd.SetOnEvent(callback)` or `wnd.AddOnEvent(callback)`. Use for one-shot actions only. For continuous input use `wnd.IsKeyHeld()`.

    enum class VXEventType : uint32_t {
        None, Quit,
        KeyDown, KeyUp,
        MouseMove, MouseDown, MouseUp, MouseWheel,
        AudioDeviceAdded, AudioDeviceRemoved,
        GamepadAdded, GamepadRemoved, GamepadAxis, GamepadButton,
        WindowResized, WindowFocusGained, WindowFocusLost, WindowMinimized, WindowRestored
    }

    struct VXKeyEvent   { VXKey keyCode; bool repeat; }
    struct VXMouseEvent { float x, y, deltaX, deltaY, wheelX, wheelY; uint8_t button; }
    struct VXAudioDeviceEvent { uint32_t deviceId; bool isCapture; }
    struct VXGamepadEvent { int32_t gamepadId; uint8_t button; int16_t axisValue; uint8_t axis; }
    struct VXWindowEvent { uint32_t width, height; }

    struct VXEvent {
        VXEventType type;
        union { VXKeyEvent key; VXMouseEvent mouse; VXAudioDeviceEvent audio;
                VXGamepadEvent gamepad; VXWindowEvent window; };

        SDL_Event ToSDL() const;   // converts VXEvent back to SDL_Event — used internally by VXOverlay
    }

Example:

    wnd.SetOnEvent([&](VX::VXEvent& event) {
        if (event.type == VX::VXEventType::MouseMove && wnd.IsMouseLocked())
            camera.Rotate(-event.mouse.deltaX * 0.1f, event.mouse.deltaY * 0.1f);

        if (event.type == VX::VXEventType::MouseDown && event.mouse.button == 1)
            wnd.LockMouse();

        if (event.type == VX::VXEventType::KeyDown && event.key.keyCode == VX::VXKey::Escape)
            wnd.FireCloseEvent();
    });

---

## VX::VXKey

Full keyboard scancode enum. Used with `wnd.IsKeyHeld()` and compared against `event.key.keyCode`.

`VXKey::Enter` is an alias for `VXKey::Return` — both are valid.

Selected values:

    A-Z         : VXKey::A through VXKey::Z
    0-9         : VXKey::Num0 through VXKey::Num9
    F1-F24      : VXKey::F1 through VXKey::F24
    Arrow keys  : VXKey::Up, VXKey::Down, VXKey::Left, VXKey::Right
    Modifiers   : VXKey::LeftShift, VXKey::RightShift, VXKey::LeftCtrl,
                  VXKey::RightCtrl, VXKey::LeftAlt, VXKey::RightAlt,
                  VXKey::LeftGui, VXKey::RightGui
    Special     : VXKey::Escape, VXKey::Return, VXKey::Enter (alias for Return),
                  VXKey::Space, VXKey::Tab, VXKey::Backspace, VXKey::Delete,
                  VXKey::Insert, VXKey::Home, VXKey::End, VXKey::PageUp, VXKey::PageDown,
                  VXKey::CapsLock, VXKey::PrintScreen, VXKey::ScrollLock, VXKey::Pause
    Numpad      : VXKey::Kp0 through VXKey::Kp9, VXKey::KpEnter,
                  VXKey::KpPlus, VXKey::KpMinus, VXKey::KpMultiply, VXKey::KpDivide,
                  VXKey::KpPeriod, VXKey::KpEquals, VXKey::Kp00, VXKey::Kp000
    Media       : VXKey::MediaNext, VXKey::MediaPrev, VXKey::MediaStop,
                  VXKey::MediaPlay, VXKey::MediaMute, VXKey::MediaSelect
    Volume      : VXKey::VolumeUp, VXKey::VolumeDown, VXKey::Mute
    Nav         : VXKey::AppBack, VXKey::AppForward

---

## VX::VXFlags

Group-partitioned flag enum using `uint64_t`. Upper bytes identify the group, lower bytes are the flag bits. Groups never collide.

    NONE                  = 0

    // Metrics group — 0x00100000
    METRICS_SCREEN_DIMENS = 0x00100001

    // Window group — 0x00200000
    WINDOW_FULLSCREEN     = 0x00200001
    WINDOW_WINDOWED       = 0x00200002
    WINDOW_BORDERLESS     = 0x00200003

    // Renderer group — 0x00300000
    RENDERER_2D           = 0x00300001
    RENDERER_3D           = 0x00300002

    // Cull mode group — 0x00400000
    CULL_NONE             = 0x00400001
    CULL_BACK             = 0x00400002
    CULL_FRONT            = 0x00400003
    CULL_FRONT_BACK       = 0x00400004

    // Mesh group — 0x00500000
    MESH_TRANSPARENT      = 0x00500001

Flags can be OR'd together within the same group:

    VX::VXFlags::WINDOW_FULLSCREEN | VX::VXFlags::WINDOW_BORDERLESS

---

## VX::VXError

    VX::VXError err = VX::GetLastError();
    std::string msg = err.what();      // lowercase 'what', not 'What'
    bool empty      = err.Empty();

    VX::SetLastError(VX::VXError("message"));

Always check `GetLastError()` after any method that returns `bool` for failure.

There is also `VX::VXRuntimeError` (extends `std::runtime_error`) for internal engine throws — game code does not construct these directly.

---

## VX::VXHaptic

Wraps controller rumble/vibration. Returned by `ctx.EnumerateHapticDevices()`.

    std::vector<VX::VXHaptic> haptics = ctx.EnumerateHapticDevices();

    haptics[0].IsSupported();
    haptics[0].Rumble(strength, durationMs);   // strength 0.0f - 1.0f
    haptics[0].Stop();
    haptics[0].GetName();

VXHaptic is NOT copyable. Move-only.

---

## VX::VXAudio

Loads and plays WAV files. Backed by SDL3 audio streams with per-instance volume, pan, and loop control. Part of the umbrella header (`VX.hpp`).

    VX::VXAudio audio(fs::path("sounds/explosion.wav"));

    audio.Play();           // plays from the beginning
    audio.Stop();           // stops and resets position
    audio.Pause();          // pauses at current position
    audio.Resume();         // resumes from paused position

    audio.SetVolume(float); // 0.0 (silent) to 1.0 (full) — clamped
    audio.GetVolume();

    audio.SetLoop(bool);    // true = loop, false = one-shot
    audio.IsLooping();

    audio.SetPan(float);    // -1.0 (left) to 1.0 (right), 0.0 center — clamped
    audio.GetPan();         // pan only affects stereo files

    audio.IsPlaying();      // true if playing AND not paused
    audio.IsPaused();

    audio.Destroy();        // explicit cleanup — also called by destructor

Pan constants for clarity:

    VX::AUDIO_PAN_LEFT   = -1.0f
    VX::AUDIO_PAN_CENTER =  0.0f
    VX::AUDIO_PAN_RIGHT  =  1.0f

VXAudio is NOT copyable. Move-only. Only WAV files are supported — no MP3/OGG.

---

## VX::VXOverlay

ImGui-backed 2D overlay rendered on top of the 3D scene. Supports text, shapes, and custom fonts. Part of the umbrella header (`VX.hpp`). Created via `ctx.CreateOverlay()` after `renderer.Populate()`.

    VX::VXOverlay overlay = ctx.CreateOverlay(wnd, gpus[0], renderer);
    bool ok = overlay.Populate();   // must be called before use

### Lifecycle

    overlay.Render();       // call every frame BEFORE renderer.Render() — records ImGui draw data
    overlay.ClearFrame();   // clears all queued text and shapes — call at start of frame if rebuilding each frame
    overlay.Destroy();      // explicit cleanup — also called by destructor

### Text

All text methods return an `int` ID that can be used to update or remove the entry later.

    int id = overlay.Text(x, y, "Hello {}", name);
    int id = overlay.Text(x, y, fontId, "Hello {}", name);

    int id = overlay.TextColored(x, y, r, g, b, a, "Hello");
    int id = overlay.TextColored(x, y, r, g, b, a, fontId, "Hello");

    int id = overlay.TextAnchored(VX::VXOverlayAnchor::TopRight, offsetX, offsetY, "FPS: {}", fps);
    int id = overlay.TextAnchored(anchor, offsetX, offsetY, fontId, "FPS: {}", fps);

    overlay.UpdateText(id, "New text {}", value);
    overlay.UpdateText(id, fontId, "New text {}", value);

    overlay.RemoveText(id);

Text uses `std::format` — format strings follow C++20 `{}` syntax.

### Shapes

    int id = overlay.Rect(x, y, w, h, r, g, b, a);                          // filled rect
    int id = overlay.RectOutline(x, y, w, h, r, g, b, a, thickness=1.0f);   // outlined rect

    int id = overlay.Line(x1, y1, x2, y2, r, g, b, a, thickness=1.0f);

    int id = overlay.Circle(x, y, radius, r, g, b, a);                      // filled circle
    int id = overlay.CircleOutline(x, y, radius, r, g, b, a, thickness=1.0f);

    int id = overlay.Polygon(points, r, g, b, a);                           // filled convex polygon
    int id = overlay.PolygonOutline(points, r, g, b, a, thickness=1.0f);

    overlay.RemoveRect(id);
    overlay.RemoveLine(id);
    overlay.RemoveCircle(id);
    overlay.RemovePolygon(id);

`points` is `std::vector<VX::VXOverlayPolygonPoint>` where each point has `float x, y`.

Color components are `float` in range 0.0–1.0.

### Fonts

    int fontId = overlay.LoadFont(fs::path("fonts/arial.ttf"), 18.0f);   // returns font ID, -1 on failure
    overlay.SetFont(fontId);    // sets active font for subsequent Text() calls
    overlay.ResetFont();        // resets to default ImGui font

### Anchors

    enum class VXOverlayAnchor {
        TopLeft, TopRight, BottomLeft, BottomRight, Free
    };

`Free` behaves the same as `TopLeft` — offset is used as absolute position.

### Important notes

- `overlay.Render()` must be called BEFORE `renderer.Render()` each frame.
- VXOverlay internally calls `wnd.AddOnEvent()` during `Populate()` to forward SDL events to ImGui. Do NOT call `wnd.SetOnEvent()` after `overlay.Populate()` — it will stomp the overlay's event hook. Use `wnd.AddOnEvent()` for any additional handlers instead.
- VXOverlay is NOT copyable. Move-only.

---

## VX::OBJModel and VX::GLTFModel

Model loaders — NOT included via VX.hpp, must be included separately.

    #include <VXGE/Model/OBJ.hpp>
    #include <VXGE/Model/GLTF.hpp>

    VX::OBJModel obj;
    if (obj.Load(fs::path("models/car.obj"))) {
        for (auto& mesh : obj.GetMeshes())
            meshes.push_back(renderer.CreateMesh(mesh.vertices, mesh.indices));
    }

    VX::GLTFModel gltf;
    if (gltf.Load(fs::path("models/character.glb"))) {
        for (auto& mesh : gltf.GetMeshes())
            meshes.push_back(renderer.CreateMesh(mesh.vertices, mesh.indices));
    }

Both have the same interface:

    bool                    model.Load(const fs::path& path);
    std::vector<MeshData>&  model.GetMeshes();   // non-const — vertices are modifiable
    bool                    model.IsLoaded();

MeshData struct:

    struct MeshData {
        std::vector<VXVertex> vertices;
        std::vector<uint32_t> indices;
        std::string           name;
    };

Normals are mapped into vertex color as a placeholder until texture support lands. To override vertex colors after loading:

    for (auto& mesh : model.GetMeshes()) {
        for (auto& vertex : mesh.vertices) {
            vertex.color[0] = 1.0f;
            vertex.color[1] = 1.0f;
            vertex.color[2] = 1.0f;
        }
        meshes.push_back(renderer.CreateMesh(mesh.vertices, mesh.indices));
    }

GLTFModel supports both `.gltf` and `.glb` — extension is detected automatically.

---

## Game Loop Pattern

This is the canonical VXGE game loop. Do not deviate from this structure.

    ctx.LockWindowContext();
    ctx.ShowWindow(wnd);

    float aspect = (float)monitors[0].Width() / (float)monitors[0].Height();
    VX::VXCamera camera(75.0f, aspect, 0.1f, 100.0f);
    camera.SetPosition({ 0.0f, 0.0f, -2.0f });

    wnd.SetOnCloseRequest([&]() -> bool {
        wnd.UnlockMouse();
        return true;
    });

    wnd.SetOnEvent([&](VX::VXEvent& event) {
        if (event.type == VX::VXEventType::MouseMove && wnd.IsMouseLocked())
            camera.Rotate(-event.mouse.deltaX * 0.1f, event.mouse.deltaY * 0.1f);
        if (event.type == VX::VXEventType::MouseDown && event.mouse.button == 1)
            wnd.LockMouse();
        if (event.type == VX::VXEventType::KeyDown && event.key.keyCode == VX::VXKey::Escape)
            wnd.FireCloseEvent();
    });

    float identity[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };

    while (!wnd.IsClosing()) {
        bool firstFrame = ctx.TickTime();
        if (firstFrame) continue;

        float dt = ctx.DeltaTime();

        wnd.PollEvents();   // must be called before IsKeyHeld

        VX::VXVec3 forward = camera.GetForward();
        VX::VXVec3 right   = camera.GetRight();
        float speed = 3.0f * dt;

        if (wnd.IsKeyHeld(VX::VXKey::LeftShift) || wnd.IsKeyHeld(VX::VXKey::RightShift))
            speed *= 8;

        if (wnd.IsKeyHeld(VX::VXKey::W)) camera.Move(forward * speed);
        if (wnd.IsKeyHeld(VX::VXKey::S)) camera.Move(forward * -speed);
        if (wnd.IsKeyHeld(VX::VXKey::A)) camera.Move(right * -speed);
        if (wnd.IsKeyHeld(VX::VXKey::D)) camera.Move(right * speed);
        if (wnd.IsKeyHeld(VX::VXKey::Space))   camera.Move({ 0.0f,  speed, 0.0f });
        if (wnd.IsKeyHeld(VX::VXKey::LeftCtrl) || wnd.IsKeyHeld(VX::VXKey::RightCtrl))
            camera.Move({ 0.0f, -speed, 0.0f });

        // If using overlay: call overlay.Render() before renderer.Render()
        overlay.Render();

        renderer.SetCamera(camera);
        renderer.Submit(mesh, material, identity);
        renderer.Render();
    }

---

## Vertex Shader Minimum Requirements

    #version 450

    layout(location = 0) in vec3 inPosition;
    layout(location = 1) in vec3 inColor;

    layout(location = 0) out vec3 fragColor;

    layout(binding = 0) uniform UBO {
        mat4 mvp;
    } ubo;

    layout(push_constant) uniform PushConstants {
        mat4 model;
    } push;

    void main() {
        gl_Position = ubo.mvp * push.model * vec4(inPosition, 1.0);
        fragColor = inColor;
    }

Fragment shader minimum:

    #version 450

    layout(location = 0) in vec3 fragColor;
    layout(location = 0) out vec4 outColor;

    void main() {
        outColor = vec4(fragColor, 1.0);
    }

---

## CMake Integration

VXGE is designed to be used as a git submodule with `add_subdirectory`. The CMake target name is `VxGameEngine`.

    add_subdirectory(vxge)
    target_link_libraries(MyGame PRIVATE VxGameEngine)

---

## Common Mistakes

- Calling `ctx.Shutdown()` before destroying graphics — always call `ctx.DestroyGraphics(gpus)` first.
- Using `gpus.clear()` instead of `ctx.DestroyGraphics(gpus)` — the latter properly calls `Destroy()` on each gpu before clearing.
- Calling `mesh.Destroy()` or `material.Destroy()` after `ctx.DestroyRenderer()` — destroy mesh and material first.
- Not calling `wnd.Populate()` before creating a renderer — Populate must be called.
- Not calling `renderer.Populate()` before the game loop — Populate must be called.
- Not calling `overlay.Populate()` before using the overlay — Populate must be called.
- Creating an overlay before `renderer.Populate()` — overlay requires the renderer's render pass, which only exists after Populate.
- Calling `wnd.SetOnEvent()` after `overlay.Populate()` — this stomps the overlay's ImGui event hook. Use `wnd.AddOnEvent()` instead.
- Calling `overlay.Render()` after `renderer.Render()` — overlay must render before the renderer submits the frame.
- Writing SDL or Vulkan calls in game code — VXGE abstracts all of that, never bypass it.
- Using `SDL_GetKeyboardState` directly — use `wnd.IsKeyHeld(VXKey)` instead.
- Using `SDL_Event` directly — use `wnd.SetOnEvent` with `VXEvent` instead.
- Putting movement input inside `SetOnEvent` — use `wnd.IsKeyHeld` in the game loop instead.
- Forgetting `renderer.SetCamera(camera)` before `renderer.Render()` — the UBO won't be updated.
- Forgetting `ctx.TickTime()` at the top of the loop — DeltaTime will always return 0.
- Copying VXGraphics, VXRenderer, VXWindow, VXHaptic, or VXAudio — all are move-only, copies are deleted.
- Using `err.What()` — the correct method is `err.what()` (lowercase).
- Using `monitor.GetWidth()` — correct method is `monitor.Width()`.
- Using `monitor.GetHeight()` — correct method is `monitor.Height()`.
- Using `wnd.GetTitle()`, `wnd.GetWidth()`, `wnd.GetSDLWindow()` — correct methods are `wnd.Title()`, `wnd.Width()`, `wnd.SDLWindow()`.
- Using `VXFlags` as `uint32_t` — it is `uint64_t`.
- Including model loaders via `VX.hpp` — they are NOT in the umbrella header, include `VXGE/Model/OBJ.hpp` and `VXGE/Model/GLTF.hpp` separately.
- Using `VXOBJ` or `VXGLTF` — the correct class names are `OBJModel` and `GLTFModel`.
- Passing `std::string` to `CreateMaterial` — it takes `fs::path`.
- Passing `std::string` to `VXAudio` constructor — it takes `fs::path`.
- Trying to copy `OBJModel::MeshData` to modify vertex colors — `GetMeshes()` returns a non-const reference, modify directly.
- Forgetting the `push_constant` block in the vertex shader — without it, per-object transforms won't work.
- Loading non-WAV files with VXAudio — only WAV is supported.

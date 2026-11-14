---
name: vxge
description: "Reference guide for writing correct C++ game code using VXGE (Vulkan-powered Game Engine). Use when working on any project that includes VXGE, when writing code that uses VX:: namespace classes, when setting up a game loop, renderer, window, camera, input, textures, or model loading with VXGE. Covers all public API methods, correct method names, initialization order, destruction order, shader requirements, and common mistakes."
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

`VX.hpp` includes `VXAudio.hpp`, `VXCamera.hpp`, `VXEngine.hpp`, `VXError.hpp`, `VXEvent.hpp`, `VXFlags.hpp`, `VXGraphics.hpp`, `VXHaptic.hpp`, `VXKey.hpp`, `VXMaterial.hpp`, `VXMath.hpp`, `VXMesh.hpp`, `VXMonitor.hpp`, `VXOverlay.hpp`, `VXRenderer.hpp`, `VXTexture.hpp`, and `VXWindow.hpp`.

Individual headers if needed:

    #include <VXGE/VXEngine.hpp>
    #include <VXGE/VXGraphics.hpp>
    #include <VXGE/VXMonitor.hpp>
    #include <VXGE/VXWindow.hpp>
    #include <VXGE/VXRenderer.hpp>
    #include <VXGE/VXMesh.hpp>
    #include <VXGE/VXMaterial.hpp>
    #include <VXGE/VXTexture.hpp>
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

    // Monitor-based (sized to the monitor) ...
    VX::VXWindow wnd = ctx.CreateWindow("Title", monitors[0], {VX::VXFlags::WINDOW_WINDOWED});
    // ... or explicit-size (use this for an actual windowed, non-monitor-filling window)
    VX::VXWindow wnd = ctx.CreateWindow("Title", 1280u, 720u, {VX::VXFlags::WINDOW_WINDOWED});
    wnd.Populate();

    // No flags parameter any more — the renderer is always 3D.
    VX::VXRenderer renderer = ctx.CreateRenderer(wnd, gpus[0]);
    renderer.Populate();

    // Optional overlay (ImGui-backed, must be created after renderer.Populate())
    VX::VXOverlay overlay = ctx.CreateOverlay(wnd, gpus[0], renderer);
    overlay.Populate();

    // ... game loop ...

    overlay.Destroy();
    texture.Destroy(gpus[0].Device());   // any VXTexture you created yourself (see VX::VXTexture below)
    mesh.Destroy(gpus[0].Device());
    material.Destroy(gpus[0].Device());
    ctx.DestroyRenderer(renderer);
    ctx.DestroyWindow(wnd);
    ctx.DestroyGraphics(gpus);   // preferred over gpus.clear() — calls Destroy() on each, then clears
    ctx.Shutdown();

Calling `ctx.Shutdown()` before destroying graphics will cause `vkDestroyDevice: Invalid device` because the Vulkan instance is destroyed before the logical devices. Use `ctx.DestroyGraphics(gpus)` instead of `gpus.clear()` — it properly calls `gpu.Destroy()` on each entry before clearing.

Destroy any `VXTexture` you created yourself (via `renderer.CreateTexture`/`CreateTextureFromFD`/`CreateTextureFromMemory`) before `ctx.DestroyRenderer(renderer)`. Do this for every texture you created, even one assigned to a material — `VXTexture::Destroy()` does **not** destroy the shared sampler (see the `VX::VXTexture` section below), so destroying one texture does not break any other texture that happens to share the same sampler.

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

    VX::VXWindow   ctx.CreateWindow(title, monitor, flags);          // sized to the monitor
    VX::VXWindow   ctx.CreateWindow(title, width, height, flags);    // explicit size — use for a real windowed window
    VX::VXRenderer ctx.CreateRenderer(window, graphics);             // no flags — renderer is always 3D
    VX::VXOverlay  ctx.CreateOverlay(window, graphics, renderer);    // must call after renderer.Populate()

Both `CreateWindow` overloads take `const std::vector<VX::VXFlags>&`, not a single bare `VXFlags` value:

    VX::VXWindow wnd = ctx.CreateWindow("Title", 1280u, 720u, {VX::VXFlags::WINDOW_WINDOWED});

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

Wraps SDL3 window and Vulkan surface creation. Has two constructor shapes, matching the two `ctx.CreateWindow` overloads: one sized to a `VXMonitor`, and one taking an explicit width/height. **Use the explicit-size overload when you actually want a windowed (non-monitor-filling) window** — the monitor-based overload sizes the window to the monitor's own resolution regardless of `WINDOW_WINDOWED`.

    // Sized to the monitor
    VX::VXWindow wnd = ctx.CreateWindow("Title", monitors[0], {VX::VXFlags::WINDOW_WINDOWED});
    // Explicit size — produces an actual windowed-sized window
    VX::VXWindow wnd = ctx.CreateWindow("Title", 1280u, 720u, {VX::VXFlags::WINDOW_WINDOWED});

    bool ok = wnd.Populate();   // finalizes SDL3 window creation — must be called before use

    bool               wnd.IsClosing();          // returns true when window close is requested
    void               wnd.PollEvents();         // must be called every frame — updates input state
    const std::string& wnd.Title();              // window title
    uint32_t           wnd.Width();              // window width in pixels
    uint32_t           wnd.Height();              // window height in pixels
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

Wraps swapchain, render pass, framebuffers, command buffers, sync objects, descriptor sets, UBO, depth buffer, and the per-material texture descriptor set layout/pool/sampler.

    VX::VXRenderer renderer = ctx.CreateRenderer(wnd, gpus[0]);   // no flags — always 3D
    bool ok = renderer.Populate();   // must be called before use

    VX::VXMesh     renderer.CreateMesh(vertices, indices);
    VX::VXMaterial renderer.CreateMaterial(vertShaderPath, fragShaderPath,
                                            flags = {VX::VXFlags::CULL_BACK});

    VX::VXTexture  renderer.CreateTexture(imagePath);                       // decode PNG/JPG/etc. from a file path
    VX::VXTexture  renderer.CreateTextureFromFD(fd, offset, size);          // decode from an open POSIX fd + byte range
    VX::VXTexture  renderer.CreateTextureFromMemory(const uint8_t* data, size_t size);
    VX::VXTexture  renderer.CreateTextureFromMemory(const std::vector<uint8_t>& data);

    void renderer.BindMaterialTextures(material);     // allocates/updates material's texture descriptor set (set=1)
                                                        // from its currently assigned Albedo texture.
                                                        // Call after material.SetAlbedo() (or with no Albedo at all)
                                                        // and before the material's first Submit().

    void renderer.Submit(mesh, material, transform);   // queues a draw call for this frame
    void renderer.SetCamera(camera);                    // uploads VP matrix to UBO
    void renderer.Render();                             // submits frame, presents, clears queue
    void renderer.Destroy();                            // explicit cleanup — also called by destructor

    void renderer.SetTargetFPS(int fps);               // enables FPS cap
    void renderer.ClearTargetFPS();                    // removes FPS cap
    void renderer.SetVSync(bool enabled);              // toggles vsync — can be called at any time
    void renderer.SetClearColor(float r, float g, float b, float a);   // sets background clear color (default 0.1, 0.1, 0.1, 1.0)

    // Internal accessors — used by VXOverlay, do not call from game code
    VkInstance       renderer.GetInstance();
    VkRenderPass     renderer.GetRenderPass();
    VkCommandBuffer  renderer.GetCurrentCommandBuffer();

### Texture loading

    // From a file path (decoded via stb_image)
    VX::VXTexture tex = renderer.CreateTexture(fs::path("textures/checker.png"));

    // From an already-open file descriptor at a byte range — e.g. a texture packed
    // inside an archive/package file. Does not take ownership of or close the fd.
    VX::VXTexture tex = renderer.CreateTextureFromFD(fd, offset, size);

    // From an in-memory byte buffer — e.g. a glTF/GLB embedded texture already
    // decoded into bytes by a model loader.
    std::vector<uint8_t> bytes = /* ... */;
    VX::VXTexture tex = renderer.CreateTextureFromMemory(bytes);
    // or, with a raw pointer + explicit size:
    VX::VXTexture tex = renderer.CreateTextureFromMemory(bytes.data(), bytes.size());

Every texture obtained this way must be bound to a material and destroyed by the caller:

    material.SetAlbedo(&tex);
    renderer.BindMaterialTextures(material);
    // ... later, before DestroyRenderer:
    tex.Destroy(gpus[0].Device());

A material with no Albedo assigned still needs `BindMaterialTextures(material)` called on it once — this gives it a valid descriptor set bound to the renderer's internal 1x1 white fallback texture, instead of leaving it with no descriptor set at all.

Shaders must be pre-compiled SPIR-V `.spv` files. VXGE does not compile GLSL at runtime. Compile with:

    glslc shader.vert -o shader.vert.spv
    glslc shader.frag -o shader.frag.spv

The vertex shader MUST have this layout (set 0 = per-frame UBO):

    layout(set = 0, binding = 0) uniform UBO { mat4 mvp; } ubo;
    layout(push_constant) uniform PushConstants { mat4 model; } push;

    void main() {
        gl_Position = ubo.mvp * push.model * vec4(inPosition, 1.0);
    }

If the material has a texture bound, the fragment shader samples it from set 1, binding 0:

    layout(set = 1, binding = 0) uniform sampler2D albedoTex;

VXRenderer is NOT copyable. Move-only.

---

## VX::VXMesh

Holds GPU vertex and index buffers. Created via `renderer.CreateMesh()`.

    std::vector<VX::VXVertex> vertices = {
        {{ 0.0f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.5f, 0.0f }},
        {{ 0.5f,  0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f }},
        {{-0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f }},
    };
    std::vector<uint32_t> indices = { 0, 1, 2 };

    VX::VXMesh mesh = renderer.CreateMesh(vertices, indices);
    mesh.Destroy(gpus[0].Device());   // must be called before DestroyRenderer

VXVertex layout:

    struct VXVertex {
        float position[3];   // location 0 in vertex shader
        float color[3];      // location 1 in vertex shader
        float uv[2];         // location 2 in vertex shader — defaults to {0.0f, 0.0f}
    };

When building a textured mesh out of shared triangle vertices (e.g. faces of a pyramid), give each triangle its own unshared vertices with clean corner UVs rather than reusing vertices with a single averaged UV across differently-oriented faces — sharing vertices across faces with different UV orientations stretches/distorts the texture sampling on those faces.

---

## VX::VXMaterial

Holds a compiled Vulkan graphics pipeline plus the engine's PBR texture slots. Created via `renderer.CreateMaterial()`.

    VX::VXMaterial material = renderer.CreateMaterial(
        fs::path("/path/to/shader.vert.spv"),
        fs::path("/path/to/shader.frag.spv"),
        {VX::VXFlags::CULL_BACK}   // optional, defaults to {VXFlags::CULL_BACK}
    );

    VkPipeline       material.GetPipeline();
    VkPipelineLayout material.GetPipelineLayout();

    // PBR texture slots. Only Albedo is currently sampled by the default pipeline/descriptor
    // layout — Normal, RoughnessMetallic and AO are carried on the material for future use by
    // loaders and content pipelines, but the renderer does not yet bind or sample them.
    void material.SetAlbedo(VX::VXTexture* texture);
    void material.SetNormal(VX::VXTexture* texture);
    void material.SetRoughnessMetallic(VX::VXTexture* texture);
    void material.SetAO(VX::VXTexture* texture);

    VX::VXTexture* material.GetAlbedo();
    VX::VXTexture* material.GetNormal();
    VX::VXTexture* material.GetRoughnessMetallic();
    VX::VXTexture* material.GetAO();

    // Per-material descriptor set (set = 1). Normally you don't touch these directly —
    // renderer.BindMaterialTextures(material) manages them for you.
    VkDescriptorSet material.GetDescriptorSet();
    void            material.SetDescriptorSet(VkDescriptorSet set);

    material.Destroy(gpus[0].Device());   // must be called before DestroyRenderer — destroys pipeline + layout only

Typical usage with a texture:

    VX::VXTexture albedo = renderer.CreateTexture(fs::path("textures/checker.png"));
    material.SetAlbedo(&albedo);
    renderer.BindMaterialTextures(material);
    // ...
    albedo.Destroy(gpus[0].Device());   // before DestroyRenderer

---

## VX::VXTexture

Wraps a Vulkan sampled image (image, view, memory) plus a sampler handle. Created via `renderer.CreateTexture()` / `renderer.CreateTextureFromFD()` / `renderer.CreateTextureFromMemory()` — never constructed directly with real data by game code.

    VkImage      texture.GetImage();
    VkImageView  texture.GetImageView();
    VkSampler    texture.GetSampler();
    uint32_t     texture.GetWidth();
    uint32_t     texture.GetHeight();

    texture.Destroy(gpus[0].Device());   // must be called before DestroyRenderer

**Sampler ownership is renderer-owned, not per-texture.** Every `VXTexture` produced by a given `VXRenderer` currently shares that renderer's single default sampler (including the renderer's internal 1x1 white fallback texture used for untextured materials). `VXTexture::Destroy()` deliberately does **not** destroy the sampler — it only destroys the image view, image, and device memory — because the sampler is destroyed exactly once by `VXRenderer::Destroy()`. Do not add sampler destruction to a texture cleanup path yourself; doing so double-frees the shared sampler handle (`free(): double free detected in tcache 2` / SIGABRT) the moment more than one texture exists at once.

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

Aspect ratio for the camera should be computed from the **window**, not the monitor — a windowed (`WINDOW_WINDOWED`) window is not the same size as the monitor:

    float aspect = (float)wnd.Width() / (float)wnd.Height();
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

Plain (non-bitmask) enum, backed by `uint64_t`. **There is no longer a bitmask group system and no `operator|`/`operator&`.** Functions that take flags now take `const std::vector<VX::VXFlags>&` instead of a single OR'd value, and membership is checked with the free function `VX::HasFlag`.

    NONE                  = 0

    METRICS_SCREEN_DIMENS = 0x00100001

    WINDOW_FULLSCREEN     = 0x00200001
    WINDOW_WINDOWED       = 0x00200002
    WINDOW_BORDERLESS     = 0x00200003

    CULL_NONE             = 0x00400001
    CULL_BACK             = 0x00400002
    CULL_FRONT            = 0x00400003
    CULL_FRONT_BACK       = 0x00400004

    MESH_TRANSPARENT      = 0x00500001

**`RENDERER_2D` and `RENDERER_3D` have been removed entirely.** The renderer is always 3D regardless of flags, and `ctx.CreateRenderer`/`VXRenderer`'s constructor no longer take a flags parameter at all.

Pass flags as a `std::vector` initializer list, not a bare value, and not OR'd together:

    // CORRECT
    ctx.CreateWindow("Title", 1280u, 720u, {VX::VXFlags::WINDOW_WINDOWED});
    renderer.CreateMaterial(vert, frag, {VX::VXFlags::CULL_NONE});

    // WRONG — these no longer compile / no longer exist
    ctx.CreateWindow("Title", monitors[0], VX::VXFlags::WINDOW_WINDOWED);              // bare value
    VX::VXFlags::WINDOW_FULLSCREEN | VX::VXFlags::WINDOW_BORDERLESS;                   // operator| removed
    ctx.CreateRenderer(wnd, gpus[0], VX::VXFlags::RENDERER_3D);                        // flag + param both gone

To check whether a vector of flags contains a given flag, use `VX::HasFlag`:

    bool HasFlag(const std::vector<VX::VXFlags>& flags, VX::VXFlags flag);

    if (VX::HasFlag(someFlags, VX::VXFlags::CULL_NONE)) { /* ... */ }

---

## VX::VXError

    VX::VXError err = VX::GetLastError();
    std::string msg = err.what();      // lowercase 'what', not 'What'
    bool empty      = err.Empty();

    VX::SetLastError(VX::VXError("message"));

Always check `GetLastError()` after any method that returns `bool` for failure, and after texture-loading calls (`CreateTexture`/`CreateTextureFromFD`/`CreateTextureFromMemory`), which return an empty `VXTexture()` on failure and set the last error instead of throwing.

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

Normals are mapped into vertex color as a placeholder until these loaders populate real UVs. To override vertex colors after loading:

    for (auto& mesh : model.GetMeshes()) {
        for (auto& vertex : mesh.vertices) {
            vertex.color[0] = 1.0f;
            vertex.color[1] = 1.0f;
            vertex.color[2] = 1.0f;
        }
        meshes.push_back(renderer.CreateMesh(mesh.vertices, mesh.indices));
    }

GLTFModel supports both `.gltf` and `.glb` — extension is detected automatically. A glTF/GLB embedded texture, once decoded to bytes by the loader, can be handed straight to `renderer.CreateTextureFromMemory(bytes)` rather than written to a temporary file first.

---

## Game Loop Pattern

This is the canonical VXGE game loop. Do not deviate from this structure.

    ctx.LockWindowContext();
    ctx.ShowWindow(wnd);

    float aspect = (float)wnd.Width() / (float)wnd.Height();
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
    layout(location = 2) in vec2 inUV;

    layout(location = 0) out vec3 fragColor;
    layout(location = 1) out vec2 fragUV;

    layout(set = 0, binding = 0) uniform UBO {
        mat4 mvp;
    } ubo;

    layout(push_constant) uniform PushConstants {
        mat4 model;
    } push;

    void main() {
        gl_Position = ubo.mvp * push.model * vec4(inPosition, 1.0);
        fragColor = inColor;
        fragUV = inUV;
    }

Fragment shader minimum (textured):

    #version 450

    layout(location = 0) in vec3 fragColor;
    layout(location = 1) in vec2 fragUV;

    layout(location = 0) out vec4 outColor;

    layout(set = 1, binding = 0) uniform sampler2D albedoTex;

    void main() {
        outColor = texture(albedoTex, fragUV) * vec4(fragColor, 1.0);
    }

If a material will never have a texture bound to it, the fragment shader can skip the `albedoTex` sampler and just output `vec4(fragColor, 1.0)` — but note that `CreateMaterial`'s pipeline layout always reserves set 1 for textures regardless, so a pipeline built this way still works correctly against a material that does have `BindMaterialTextures` called on it (sampling the renderer's white fallback texture).

---

## CMake Integration

VXGE is designed to be used as a git submodule with `add_subdirectory`. The CMake target name is `VxGameEngine`.

    add_subdirectory(vxge)
    target_link_libraries(MyGame PRIVATE VxGameEngine)

VXGE vendors `stb_image.h` (single-header, `External/stb/`) for texture decoding (PNG/JPG/etc.) — no extra dependency needed in consuming projects, it's compiled into `VxGameEngine` itself.

---

## Common Mistakes

- Calling `ctx.Shutdown()` before destroying graphics — always call `ctx.DestroyGraphics(gpus)` first.
- Using `gpus.clear()` instead of `ctx.DestroyGraphics(gpus)` — the latter properly calls `Destroy()` on each gpu before clearing.
- Calling `mesh.Destroy()`, `material.Destroy()`, or `texture.Destroy()` after `ctx.DestroyRenderer()` — destroy mesh, material, and any textures first.
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
- Passing a bare `VXFlags` value (e.g. `VX::VXFlags::WINDOW_WINDOWED`) instead of a `std::vector<VXFlags>`/initializer list (e.g. `{VX::VXFlags::WINDOW_WINDOWED}`) — every function that takes flags takes a vector now.
- Using `operator|` or `operator&` on `VXFlags` — these were removed. Use `VX::HasFlag(flagsVector, flag)` to check membership, and build a vector literal to combine flags.
- Referencing `VXFlags::RENDERER_2D` or `VXFlags::RENDERER_3D` — both were removed entirely; the renderer is always 3D.
- Passing a flags argument to `ctx.CreateRenderer(...)` / `VXRenderer`'s constructor — neither takes one any more.
- Computing camera aspect ratio from the monitor (`monitors[0].Width()/Height()`) instead of the window (`wnd.Width()/Height()`) — these differ for a `WINDOW_WINDOWED` window.
- Expecting `ctx.CreateWindow(title, monitor, flags)` to produce a smaller windowed window via `WINDOW_WINDOWED` alone — that overload sizes to the monitor regardless of the flag; use the explicit `ctx.CreateWindow(title, width, height, flags)` overload for an actual windowed-sized window.
- Including model loaders via `VX.hpp` — they are NOT in the umbrella header, include `VXGE/Model/OBJ.hpp` and `VXGE/Model/GLTF.hpp` separately.
- Using `VXOBJ` or `VXGLTF` — the correct class names are `OBJModel` and `GLTFModel`.
- Passing `std::string` to `CreateMaterial` — it takes `fs::path`.
- Passing `std::string` to `VXAudio` constructor — it takes `fs::path`.
- Trying to copy `OBJModel::MeshData` to modify vertex colors — `GetMeshes()` returns a non-const reference, modify directly.
- Forgetting the `push_constant` block in the vertex shader — without it, per-object transforms won't work.
- Loading non-WAV files with VXAudio — only WAV is supported.
- Assuming `VXTexture::Destroy()` destroys the sampler — it doesn't; the sampler is renderer-owned and shared across every texture that renderer created, and is destroyed exactly once by `VXRenderer::Destroy()`. Destroying it per-texture double-frees it.
- Forgetting to call `renderer.BindMaterialTextures(material)` — a material's texture descriptor set (set = 1) is not created automatically just by calling `SetAlbedo()`; you must call `BindMaterialTextures` afterward (or at all, even with no albedo) before the material is first submitted.
- Creating a mesh with shared vertices across differently-UV'd triangle faces (e.g. a naively-shared pyramid apex/base) — this distorts the sampled texture on those faces. Give each face its own unshared vertices with clean per-corner UVs.
- Forgetting `inUV`/`fragUV` (location 2 / location 1 out) in the vertex shader, or the `vec2 fragUV` varying in the fragment shader, when writing a shader meant to be used with `VXMaterial`'s textured pipeline — the vertex attribute layout `CreateMaterial` builds expects a 3rd attribute (location 2, `VK_FORMAT_R32G32_SFLOAT`) bound to `VXVertex::uv`.

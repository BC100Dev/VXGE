#include <VXGE/VXEngine.hpp>
#include <VXGE/VXError.hpp>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <chrono>

namespace VX {
    static auto vx_lastTime = std::chrono::high_resolution_clock::now();
    static float vx_deltaTime = 0.0f;

    const std::vector<const char*> VXEngine::VALIDATION_LAYERS = {
        "VK_LAYER_KHRONOS_validation"
    };

    VXEngine::VXEngine(const std::string& appName) {
        m_appName = appName;
    }

    VXEngine::~VXEngine() {
        Shutdown();
    }

    bool VXEngine::Initialize() {
        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD | SDL_INIT_HAPTIC | SDL_INIT_JOYSTICK)) {
            SetLastError(VXError(SDL_GetError()));
            return false;
        }

        createInstance();
        return m_instance != VK_NULL_HANDLE;
    }

    void VXEngine::Shutdown() {
        if (m_instance != VK_NULL_HANDLE) {
            vkDestroyInstance(m_instance, nullptr);
            m_instance = VK_NULL_HANDLE;
        }

        SDL_Quit();
    }

    void VXEngine::createInstance() {
        uint32_t extCount = 0;
        const char* const* sdlExtensions = SDL_Vulkan_GetInstanceExtensions(&extCount);
        if (!sdlExtensions) {
            SetLastError(VXError(SDL_GetError()));
            return;
        }

        std::vector extensions(sdlExtensions, sdlExtensions + extCount);

        if (ENABLE_VALIDATION)
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = m_appName.c_str();
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "VXGE";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        if (ENABLE_VALIDATION) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
            createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
        }

        if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS)
            SetLastError(VXError("Failed to create Vulkan instance"));
    }

    std::vector<VXGraphics> VXEngine::EnumerateGraphicsCards() {
        std::vector<VXGraphics> cards;

        uint32_t count = 0;
        vkEnumeratePhysicalDevices(m_instance, &count, nullptr);
        std::vector<VkPhysicalDevice> devices(count);
        vkEnumeratePhysicalDevices(m_instance, &count, devices.data());

        for (const auto& device : devices)
            cards.emplace_back(device);

        return cards;
    }

    std::vector<VXMonitor> VXEngine::EnumerateMonitors() {
        std::vector<VXMonitor> monitors;

        int displayCount = 0;
        SDL_DisplayID* displays = SDL_GetDisplays(&displayCount);

        for (int i = 0; i < displayCount; i++)
            monitors.emplace_back(displays[i]);

        SDL_free(displays);
        return monitors;
    }

    VXWindow VXEngine::CreateWindow(const std::string& title, const VXMonitor& monitor, VXFlags flags) {
        if (m_contextLocked) {
            SetLastError(VXError("Window and Renderer context have been locked"));
            return {};
        }

        return VXWindow(title, monitor, flags);
    }

    VXRenderer VXEngine::CreateRenderer(VXWindow& window, VXGraphics& graphics, VXFlags flags) {
        if (m_contextLocked) {
            SetLastError(VXError("Window and Renderer context have been locked"));
            return {};
        }

        return VXRenderer(window, graphics, m_instance, flags);
    }

    void VXEngine::ShowWindow(VXWindow& window) {
        SDL_ShowWindow(window.SDLWindow());
    }

    void VXEngine::LockWindowContext() {
        m_contextLocked = true;
    }

    void VXEngine::DestroyWindow(VXWindow& window) {
        SDL_DestroyWindow(window.SDLWindow());
    }

    void VXEngine::DestroyRenderer(VXRenderer& renderer) {
        renderer.Destroy();
    }

    std::vector<VXHaptic> VXEngine::EnumerateHapticDevices() {
        std::vector<VXHaptic> devices;

        int count = 0;
        SDL_JoystickID* joysticks = SDL_GetJoysticks(&count);
        if (!joysticks) return devices;

        for (int i = 0; i < count; i++)
            if (SDL_IsJoystickHaptic(SDL_OpenJoystick(joysticks[i])))
                devices.emplace_back(joysticks[i]);

        SDL_free(joysticks);
        return devices;
    }

    void VXEngine::TickTime() {
        auto now = std::chrono::high_resolution_clock::now();
        vx_deltaTime = std::chrono::duration<float>(now - vx_lastTime).count();
        vx_lastTime = now;
    }

    float VXEngine::DeltaTime() const {
        return vx_deltaTime;
    }
}

#include <VXGE/VXOverlay.hpp>
#include <VXGE/VXError.hpp>
#include "VXOverlayData.hpp"

#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_vulkan.h>

#include <algorithm>
#include <utility>

namespace VX {
    VXOverlay::VXOverlay(VXWindow& window, VXGraphics& graphics,
                         VkInstance instance, VkRenderPass renderPass)
        : m_instance(instance),
          m_physicalDevice(graphics.PhysicalDevice()),
          m_device(graphics.Device()),
          m_queue(graphics.GraphicsQueue()),
          m_queueFamily(graphics.GraphicsFamily()),
          m_window(window.SDLWindow()),
          m_vxWindow(&window),
          m_renderPass(renderPass) {
        ovData.overlay = this;
    }

    VXOverlay::~VXOverlay() { Destroy(); }

    VXOverlay::VXOverlay(VXOverlay&& other) noexcept
        : m_instance(other.m_instance), m_physicalDevice(other.m_physicalDevice),
          m_device(other.m_device), m_queue(other.m_queue),
          m_queueFamily(other.m_queueFamily), m_window(other.m_window),
          m_vxWindow(other.m_vxWindow), m_descriptorPool(other.m_descriptorPool),
          m_renderPass(other.m_renderPass), m_ready(other.m_ready),
          m_nextId(other.m_nextId), m_activeFont(other.m_activeFont),
          m_texts(std::move(other.m_texts)), m_rects(std::move(other.m_rects)),
          m_lines(std::move(other.m_lines)), m_circles(std::move(other.m_circles)),
          m_fonts(std::move(other.m_fonts)) {
        other.m_instance = VK_NULL_HANDLE;
        other.m_physicalDevice = VK_NULL_HANDLE;
        other.m_device = VK_NULL_HANDLE;
        other.m_queue = VK_NULL_HANDLE;
        other.m_queueFamily = 0;
        other.m_window = nullptr;
        other.m_vxWindow = nullptr;
        other.m_descriptorPool = VK_NULL_HANDLE;
        other.m_renderPass = VK_NULL_HANDLE;
        other.m_ready = false;
        other.m_nextId = 1;
        other.m_activeFont = -1;
        ovData.overlay = this;
    }

    VXOverlay& VXOverlay::operator=(VXOverlay&& other) noexcept {
        if (this != &other) {
            Destroy();
            m_instance = other.m_instance;
            m_physicalDevice = other.m_physicalDevice;
            m_device = other.m_device;
            m_queue = other.m_queue;
            m_queueFamily = other.m_queueFamily;
            m_window = other.m_window;
            m_vxWindow = other.m_vxWindow;
            m_descriptorPool = other.m_descriptorPool;
            m_renderPass = other.m_renderPass;
            m_ready = other.m_ready;
            m_nextId = other.m_nextId;
            m_activeFont = other.m_activeFont;
            m_texts = std::move(other.m_texts);
            m_rects = std::move(other.m_rects);
            m_lines = std::move(other.m_lines);
            m_circles = std::move(other.m_circles);
            m_fonts = std::move(other.m_fonts);
            other.m_instance = VK_NULL_HANDLE;
            other.m_physicalDevice = VK_NULL_HANDLE;
            other.m_device = VK_NULL_HANDLE;
            other.m_queue = VK_NULL_HANDLE;
            other.m_queueFamily = 0;
            other.m_window = nullptr;
            other.m_vxWindow = nullptr;
            other.m_descriptorPool = VK_NULL_HANDLE;
            other.m_renderPass = VK_NULL_HANDLE;
            other.m_ready = false;
            other.m_nextId = 1;
            other.m_activeFont = -1;
            ovData.overlay = this;
        }
        return *this;
    }

    bool VXOverlay::Populate() {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
        io.IniFilename = nullptr;
        ImGui::StyleColorsDark();

        if (!ImGui_ImplSDL3_InitForVulkan(m_window)) {
            SetLastError(VXError("VXOverlay: ImGui SDL3 backend init failed"));
            return false;
        }

        VkDescriptorPoolSize poolSizes[] = {
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 8},
            {VK_DESCRIPTOR_TYPE_SAMPLER, 8},
            {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 8}
        };
        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        poolInfo.maxSets = 8;
        poolInfo.poolSizeCount = 3;
        poolInfo.pPoolSizes = poolSizes;
        vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool);

        ImGui_ImplVulkan_InitInfo initInfo{};
        initInfo.Instance = m_instance;
        initInfo.PhysicalDevice = m_physicalDevice;
        initInfo.Device = m_device;
        initInfo.QueueFamily = m_queueFamily;
        initInfo.Queue = m_queue;
        initInfo.DescriptorPool = m_descriptorPool;
        initInfo.MinImageCount = 2;
        initInfo.ImageCount = 2;
        initInfo.PipelineInfoMain.RenderPass = m_renderPass;
        initInfo.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

        if (!ImGui_ImplVulkan_Init(&initInfo)) {
            SetLastError(VXError("VXOverlay: ImGui Vulkan backend init failed"));
            vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
            m_descriptorPool = VK_NULL_HANDLE;
            ImGui_ImplSDL3_Shutdown();
            ImGui::DestroyContext();
            return false;
        }

        m_vxWindow->AddOnEvent([](VXEvent& event) {
            SDL_Event sdlEvent = event.ToSDL();
            if (sdlEvent.type != 0)
                ImGui_ImplSDL3_ProcessEvent(&sdlEvent);
        });

        m_ready = true;
        return true;
    }

    void VXOverlay::Destroy() {
        if (!m_ready) return;
        vkDeviceWaitIdle(m_device);
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
        vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
        m_descriptorPool = VK_NULL_HANDLE;
        m_fonts.clear();
        ovData.overlay = nullptr;
        m_ready = false;
    }

    int VXOverlay::LoadFont(const fs::path& path, float size) {
        ImGuiIO& io = ImGui::GetIO();
        ImFont* font = io.Fonts->AddFontFromFileTTF(path.string().c_str(), size);
        if (!font) {
            SetLastError(VXError("VXOverlay: Failed to load font: " + path.string()));
            return -1;
        }
        int id = static_cast<int>(m_fonts.size());
        m_fonts.push_back(font);
        return id;
    }

    void VXOverlay::SetFont(int id) {
        m_activeFont = id;
    }

    void VXOverlay::ResetFont() {
        m_activeFont = -1;
    }

    void VXOverlay::RemoveText(int id) {
        m_texts.erase(
            std::remove_if(m_texts.begin(), m_texts.end(),
                           [id](const VXOverlayEntry& e) { return e.id == id; }),
            m_texts.end());
    }

    int VXOverlay::Rect(float x, float y, float w, float h,
                        float r, float g, float b, float a) {
        int id = m_nextId++;
        m_rects.push_back({id, x, y, w, h, r, g, b, a, 0.0f});
        return id;
    }

    int VXOverlay::RectOutline(float x, float y, float w, float h,
                               float r, float g, float b, float a, float thickness) {
        int id = m_nextId++;
        m_rects.push_back({id, x, y, w, h, r, g, b, a, thickness});
        return id;
    }

    int VXOverlay::Line(float x1, float y1, float x2, float y2,
                        float r, float g, float b, float a, float thickness) {
        int id = m_nextId++;
        m_lines.push_back({id, x1, y1, x2, y2, r, g, b, a, thickness});
        return id;
    }

    int VXOverlay::Circle(float x, float y, float radius,
                          float r, float g, float b, float a) {
        int id = m_nextId++;
        m_circles.push_back({id, x, y, radius, r, g, b, a, 0.0f});
        return id;
    }

    int VXOverlay::CircleOutline(float x, float y, float radius,
                                 float r, float g, float b, float a, float thickness) {
        int id = m_nextId++;
        m_circles.push_back({id, x, y, radius, r, g, b, a, thickness});
        return id;
    }

    void VXOverlay::RemoveRect(int id) {
        m_rects.erase(
            std::remove_if(m_rects.begin(), m_rects.end(),
                           [id](const VXOverlayRect& e) { return e.id == id; }),
            m_rects.end());
    }

    void VXOverlay::RemoveLine(int id) {
        m_lines.erase(
            std::remove_if(m_lines.begin(), m_lines.end(),
                           [id](const VXOverlayLine& e) { return e.id == id; }),
            m_lines.end());
    }

    void VXOverlay::RemoveCircle(int id) {
        m_circles.erase(
            std::remove_if(m_circles.begin(), m_circles.end(),
                           [id](const VXOverlayCircle& e) { return e.id == id; }),
            m_circles.end());
    }

    int VXOverlay::Polygon(const std::vector<VXOverlayPolygonPoint>& points,
                           float r, float g, float b, float a) {
        int id = m_nextId++;
        m_polygons.push_back({id, points, r, g, b, a, 0.0f});
        return id;
    }

    int VXOverlay::PolygonOutline(const std::vector<VXOverlayPolygonPoint>& points,
                                  float r, float g, float b, float a, float thickness) {
        int id = m_nextId++;
        m_polygons.push_back({id, points, r, g, b, a, thickness});
        return id;
    }

    void VXOverlay::RemovePolygon(int id) {
        m_polygons.erase(
            std::remove_if(m_polygons.begin(), m_polygons.end(),
                           [id](const VXOverlayPolygon& e) { return e.id == id; }),
            m_polygons.end());
    }

    void VXOverlay::Render() {
        if (!m_ready) return;

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        ImGuiIO& io = ImGui::GetIO();
        ImGui::SetNextWindowPos({0, 0});
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::SetNextWindowBgAlpha(0.0f);
        ImGui::Begin("##vxoverlay", nullptr,
                     ImGuiWindowFlags_NoDecoration |
                     ImGuiWindowFlags_NoInputs |
                     ImGuiWindowFlags_NoNav |
                     ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoBringToFrontOnFocus |
                     ImGuiWindowFlags_NoBackground);

        ImDrawList* draw = ImGui::GetWindowDrawList();

        for (const auto& rect : m_rects) {
            ImU32 col = IM_COL32((int)(rect.r*255), (int)(rect.g*255),
                                 (int)(rect.b*255), (int)(rect.a*255));
            if (rect.thickness == 0.0f)
                draw->AddRectFilled({rect.x, rect.y},
                                    {rect.x + rect.w, rect.y + rect.h}, col);
            else
                draw->AddRect({rect.x, rect.y},
                              {rect.x + rect.w, rect.y + rect.h},
                              col, 0.0f, 0, rect.thickness);
        }

        for (const auto& poly : m_polygons) {
            std::vector<ImVec2> verts;
            verts.reserve(poly.points.size());
            for (const auto& p : poly.points)
                verts.push_back({p.x, p.y});
            ImU32 col = IM_COL32((int)(poly.r*255), (int)(poly.g*255),
                                 (int)(poly.b*255), (int)(poly.a*255));
            if (poly.thickness == 0.0f)
                draw->AddConvexPolyFilled(verts.data(), (int)verts.size(), col);
            else
                draw->AddPolyline(verts.data(), (int)verts.size(), col, ImDrawFlags_Closed, poly.thickness);
        }

        for (const auto& line : m_lines) {
            ImU32 col = IM_COL32((int)(line.r*255), (int)(line.g*255),
                                 (int)(line.b*255), (int)(line.a*255));
            draw->AddLine({line.x1, line.y1}, {line.x2, line.y2}, col, line.thickness);
        }

        for (const auto& circle : m_circles) {
            ImU32 col = IM_COL32((int)(circle.r*255), (int)(circle.g*255),
                                 (int)(circle.b*255), (int)(circle.a*255));
            if (circle.thickness == 0.0f)
                draw->AddCircleFilled({circle.x, circle.y}, circle.radius, col);
            else
                draw->AddCircle({circle.x, circle.y}, circle.radius, col, 0, circle.thickness);
        }

        for (const auto& e : m_texts) {
            ImFont* font = nullptr;
            if (e.fontIdentifier >= 0 && e.fontIdentifier < static_cast<int>(m_fonts.size()))
                font = static_cast<ImFont*>(m_fonts[e.fontIdentifier]);
            draw->AddText(font, font ? font->GetFontBaked(font->LegacySize)->Size : 0.0f,
                          {e.x, e.y},
                          IM_COL32((int)(e.r*255), (int)(e.g*255),
                                   (int)(e.b*255), (int)(e.a*255)),
                          e.text.c_str());
        }

        ImGui::End();
        ImGui::Render();
    }

    void VXOverlay::ClearFrame() {
        m_texts.clear();
        m_rects.clear();
        m_lines.clear();
        m_circles.clear();
        m_polygons.clear();
        m_nextId = 1;
    }

    void VXOverlay::resolveAnchor(VXOverlayAnchor anchor, float offsetX, float offsetY,
                                  float& outX, float& outY) const {
        ImGuiIO& io = ImGui::GetIO();
        float sw = io.DisplaySize.x;
        float sh = io.DisplaySize.y;
        switch (anchor) {
        case VXOverlayAnchor::TopLeft: outX = offsetX;
            outY = offsetY;
            break;
        case VXOverlayAnchor::TopRight: outX = sw - offsetX;
            outY = offsetY;
            break;
        case VXOverlayAnchor::BottomLeft: outX = offsetX;
            outY = sh - offsetY;
            break;
        case VXOverlayAnchor::BottomRight: outX = sw - offsetX;
            outY = sh - offsetY;
            break;
        default: outX = offsetX;
            outY = offsetY;
            break;
        }
    }
}

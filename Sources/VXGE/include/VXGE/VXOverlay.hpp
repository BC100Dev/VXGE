#ifndef VXGE_VXOVERLAY_HPP
#define VXGE_VXOVERLAY_HPP

#include <filesystem>
#include <format>
#include <string>
#include <vector>
#include <cstdarg>
#include "VXWindow.hpp"
#include "VXGraphics.hpp"

namespace fs = std::filesystem;

namespace VX {
    enum class VXOverlayAnchor {
        TopLeft, TopRight, BottomLeft, BottomRight, Free
    };

    struct VXOverlayEntry {
        int id;
        int fontIdentifier;
        float x, y, r, g, b, a;
        std::string text;
    };

    struct VXOverlayRect {
        int id;
        float x, y, w, h, r, g, b, a, thickness;
    };

    struct VXOverlayLine {
        int id;
        float x1, y1, x2, y2, r, g, b, a, thickness;
    };

    struct VXOverlayCircle {
        int id;
        float x, y, radius, r, g, b, a, thickness;
    };

    struct VXOverlayPolygonPoint {
        float x, y;
    };

    struct VXOverlayPolygon {
        int id;
        std::vector<VXOverlayPolygonPoint> points;
        float r, g, b, a, thickness;
    };

    class VXOverlay {
    public:
        VXOverlay() = default;
        ~VXOverlay();

        VXOverlay(const VXOverlay&) = delete;
        VXOverlay& operator=(const VXOverlay&) = delete;
        VXOverlay(VXOverlay&&) noexcept;
        VXOverlay& operator=(VXOverlay&&) noexcept;

        bool Populate();
        void Destroy();

        template <typename... Args>
        int Text(float x, float y, std::format_string<Args...> fmt, Args&&... args) {
            return Text(x, y, m_activeFont, fmt, args...);
        }

        template <typename... Args>
        int Text(float x, float y, int font, std::format_string<Args...> fmt, Args&&... args) {
            int id = m_nextId++;
            m_texts.push_back({
                id, font, x, y, 1.0f, 1.0f, 1.0f, 1.0f,
                std::format(fmt, std::forward<Args>(args)...)
            });
            return id;
        }

        template <typename... Args>
        int TextColored(float x, float y, float r, float g, float b, float a,
                                   std::format_string<Args...> fmt, Args&&... args) {
            return TextColored(x, y, r, g, b, a, m_activeFont, fmt, args...);
        }

        template <typename... Args>
        int TextColored(float x, float y, float r, float g, float b, float a,
                                   int font, std::format_string<Args...> fmt, Args&&... args) {
            int id = m_nextId++;
            m_texts.push_back({
                id, font, x, y, r, g, b, a,
                std::format(fmt, std::forward<Args>(args)...)
            });
            return id;
        }

        template <typename... Args>
        int TextAnchored(VXOverlayAnchor anchor, float offsetX, float offsetY,
                                    std::format_string<Args...> fmt, Args&&... args) {
            return TextAnchored(anchor, offsetX, offsetY, m_activeFont, fmt, args...);
        }

        template <typename... Args>
        int TextAnchored(VXOverlayAnchor anchor, float offsetX, float offsetY,
                                    int font, std::format_string<Args...> fmt, Args&&... args) {
            float x, y;
            resolveAnchor(anchor, offsetX, offsetY, x, y);
            int id = m_nextId++;
            m_texts.push_back({
                id, font, x, y, 1.0f, 1.0f, 1.0f, 1.0f,
                std::format(fmt, std::forward<Args>(args)...)
            });
            return id;
        }

        template <typename... Args>
        void UpdateText(int id, std::format_string<Args...> fmt, Args&&... args) {
            UpdateText(id, m_activeFont, fmt, args...);
        }

        template <typename... Args>
        void UpdateText(int id, int font, std::format_string<Args...> fmt, Args&&... args) {
            std::string text = std::format(fmt, std::forward<Args>(args)...);
            for (auto& e : m_texts)
                if (e.id == id) {
                    e.text = text;
                    e.fontIdentifier = font;
                    return;
                }
        }

        void RemoveText(int id);

        int Rect(float x, float y, float w, float h, float r, float g, float b, float a);
        int RectOutline(float x, float y, float w, float h, float r, float g, float b, float a, float thickness = 1.0f);
        int Line(float x1, float y1, float x2, float y2, float r, float g, float b, float a, float thickness = 1.0f);
        int Circle(float x, float y, float radius, float r, float g, float b, float a);
        int CircleOutline(float x, float y, float radius, float r, float g, float b, float a, float thickness = 1.0f);
        void RemoveRect(int id);
        void RemoveLine(int id);
        void RemoveCircle(int id);
        int Polygon(const std::vector<VXOverlayPolygonPoint>& points, float r, float g, float b, float a);
        int PolygonOutline(const std::vector<VXOverlayPolygonPoint>& points, float r, float g, float b, float a,
                           float thickness = 1.0f);
        void RemovePolygon(int id);

        void Render();
        void ClearFrame();

        int LoadFont(const fs::path& path, float size);
        void SetFont(int id);
        void ResetFont();

    private:
        VXOverlay(VXWindow& window, VXGraphics& graphics, VkInstance instance, VkRenderPass renderPass);

        VkInstance m_instance = VK_NULL_HANDLE;
        VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
        VkDevice m_device = VK_NULL_HANDLE;
        VkQueue m_queue = VK_NULL_HANDLE;
        uint32_t m_queueFamily = 0;
        SDL_Window* m_window = nullptr;
        VXWindow* m_vxWindow = nullptr;
        VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
        VkRenderPass m_renderPass = VK_NULL_HANDLE;
        bool m_ready = false;
        int m_nextId = 1;
        int m_activeFont = -1;

        std::vector<VXOverlayEntry> m_texts;
        std::vector<VXOverlayRect> m_rects;
        std::vector<VXOverlayLine> m_lines;
        std::vector<VXOverlayCircle> m_circles;
        std::vector<VXOverlayPolygon> m_polygons;
        std::vector<void*> m_fonts;

        void resolveAnchor(VXOverlayAnchor anchor, float offsetX, float offsetY,
                           float& outX, float& outY) const;

        friend class VXEngine;
    };
}

#endif // VXGE_VXOVERLAY_HPP

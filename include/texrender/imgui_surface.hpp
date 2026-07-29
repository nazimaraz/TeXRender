#pragma once

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <imgui.h>
#include "texrender/draw_surface.hpp"
#include "texrender/export.hpp"

#ifndef IMGUI_HAS_TEXTURES
#   error "texrender: the imgui backend needs Dear ImGui 1.92 or newer (ImGuiBackendFlags_RendererHasTextures)."
#endif

namespace TeXRender
{
    class RenderHandle;

    // Frees glyph atlases retired by destroyed surfaces once the renderer backend has released them.
    // Called internally while glyphs are drawn; call it directly when a document is destroyed but
    // drawing continues, or after the renderer backend's Shutdown() and before
    // ImGui::DestroyContext(). Call it from the thread that runs ImGui.
    TEXRENDER_EXPORT void imgui_collect_retired_textures() noexcept;

    class TEXRENDER_EXPORT ImGuiSurface final : public IDrawSurface
    {
    public:
        static constexpr auto initial_atlas_size = 512;
        static constexpr auto default_max_atlas_size = 4096;
        // Keeps the atlas within what ImGui's texture upload requests and 32-bit size math allow.
        static constexpr auto largest_atlas_size = 16384;

        ImGuiSurface();
        explicit ImGuiSurface(ImDrawList* draw_list);
        ImGuiSurface(const ImGuiSurface&) = delete;
        ImGuiSurface(ImGuiSurface&&) = delete;
        ImGuiSurface& operator=(const ImGuiSurface&) = delete;
        ImGuiSurface& operator=(ImGuiSurface&&) = delete;
        ~ImGuiSurface() noexcept override;

        void set_color(std::uint32_t argb) override;
        void draw_line(float x1, float y1, float x2, float y2, float width) override;
        void draw_rect(float x, float y, float w, float h, float width) override;
        void fill_rect(float x, float y, float w, float h) override;
        void draw_round_rect(float x, float y, float w, float h, float rx, float ry, float width) override;
        void fill_round_rect(float x, float y, float w, float h, float rx, float ry) override;
        void draw_glyph(const GlyphBitmap& glyph, float pen_x, float pen_y, float angle) override;

        // Target draw list. When null (the default) the current window's draw list is resolved at
        // draw time, so formulas land in whichever window is open.
        void set_draw_list(ImDrawList* draw_list) noexcept;
        [[nodiscard]] ImDrawList* draw_list() const noexcept;

        // Screen-space offset added to every draw. ImGui draw lists take screen coordinates, so set
        // this to ImGui::GetCursorScreenPos() to place formulas with window-local coordinates.
        void set_origin(ImVec2 origin) noexcept;
        [[nodiscard]] ImVec2 origin() const noexcept;

        void set_max_atlas_size(int size);
        [[nodiscard]] int max_atlas_size() const noexcept;
        [[nodiscard]] int atlas_size() const noexcept;
        [[nodiscard]] std::size_t cached_glyphs() const noexcept;
        void unload() noexcept;

        // False until a renderer backend advertising ImGuiBackendFlags_RendererHasTextures is
        // initialized; glyphs are skipped while it is false.
        [[nodiscard]] static bool textures_supported() noexcept;

    private:
        struct AtlasRect
        {
            int x{0};
            int y{0};
            int w{0};
            int h{0};
        };

        [[nodiscard]] ImDrawList* target_list() const noexcept;
        [[nodiscard]] ImVec2 point(float x, float y) const noexcept;
        [[nodiscard]] const AtlasRect* rect_for(const GlyphBitmap& glyph);
        [[nodiscard]] bool create_atlas(int size);
        [[nodiscard]] bool pack(int w, int h, AtlasRect& packed) noexcept;
        void blit(const GlyphBitmap& glyph, const AtlasRect& packed) const noexcept;
        void retire_atlas() noexcept;

        ImDrawList* draw_list_{};
        ImVec2 origin_{0.f, 0.f};
        ImTextureData* atlas_{};
        std::unordered_map<std::uint64_t, AtlasRect> rects_;
        int shelf_x_{0};
        int shelf_y_{0};
        int shelf_height_{0};
        int max_atlas_size_{default_max_atlas_size};
        ImU32 color_{IM_COL32_BLACK};
    };

    // Reserves ImGui layout space for a formula at the current cursor position and draws it there.
    TEXRENDER_EXPORT void imgui_formula(ImGuiSurface& surface, const RenderHandle& formula);
} // namespace TeXRender

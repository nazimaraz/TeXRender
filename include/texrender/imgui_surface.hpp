#pragma once

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>
#include <imgui.h>
#include "texrender/draw_surface.hpp"
#include "texrender/export.hpp"

#ifndef IMGUI_HAS_TEXTURES
#   error "texrender: the imgui backend needs Dear ImGui 1.92 or newer (ImGuiBackendFlags_RendererHasTextures)."
#endif

struct ImGuiContextHook;

namespace TeXRender
{
    class RenderHandle;

    // Frees glyph atlases retired in the current ImGui context once its renderer backend has released
    // them. Called internally once per frame; call it directly after the renderer backend's
    // Shutdown() and before ImGui::DestroyContext() to release the last ones. Atlases still pending
    // when a context is destroyed are released by that context's shutdown. Call it from the thread
    // that runs ImGui.
    TEXRENDER_EXPORT void imgui_collect_retired_textures() noexcept;

    // Glyphs are packed into atlas pages registered with the current ImGui context, so any renderer
    // backend that sets ImGuiBackendFlags_RendererHasTextures uploads them. A surface follows the
    // context it draws in; moving it to another context starts fresh pages there.
    class TEXRENDER_EXPORT ImGuiSurface final : public IDrawSurface
    {
    public:
        static constexpr auto initial_atlas_size = 512;
        static constexpr auto default_max_atlas_size = 4096;
        // Keeps the atlas within what ImGui's texture upload requests and 32-bit size math allow.
        static constexpr auto largest_atlas_size = 16384;
        // Pages no glyph was drawn from for this many frames are released.
        static constexpr auto stale_page_frames = 120;

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
        // Follows io.DisplayFramebufferScale, so glyphs match the density of ImGui's own text.
        [[nodiscard]] float glyph_scale() const noexcept override;

        // Target draw list. When null (the default) the current window's draw list is resolved at
        // draw time, so formulas land in whichever window is open.
        void set_draw_list(ImDrawList* draw_list) noexcept;
        [[nodiscard]] ImDrawList* draw_list() const noexcept;

        // Screen-space offset added to every draw; defaults to zero. ImGui draw lists take screen
        // coordinates, so RenderHandle::draw positions are screen positions unless this is set.
        void set_origin(ImVec2 origin) noexcept;
        [[nodiscard]] ImVec2 origin() const noexcept;

        void set_max_atlas_size(int size);
        [[nodiscard]] int max_atlas_size() const noexcept;
        // Edge length of the newest atlas page, or 0 before any glyph was drawn.
        [[nodiscard]] int atlas_size() const noexcept;
        [[nodiscard]] std::size_t atlas_pages() const noexcept;
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

        struct Page
        {
            ImTextureData* texture{};
            int shelf_x{0};
            int shelf_y{0};
            int shelf_height{0};
            int last_used_frame{0};
        };

        struct Slot
        {
            ImTextureData* texture{};
            AtlasRect rect;
        };

        static void on_context_shutdown(ImGuiContext* context, ImGuiContextHook* hook);

        [[nodiscard]] ImDrawList* target_list() const noexcept;
        [[nodiscard]] ImVec2 point(float x, float y) const noexcept;
        [[nodiscard]] const Slot* slot_for(const GlyphBitmap& glyph);
        void begin_frame(int frame);
        [[nodiscard]] Page* add_page(int size, int frame);
        [[nodiscard]] Page* page_of(const ImTextureData* texture) noexcept;
        [[nodiscard]] static bool pack(Page& page, int w, int h, AtlasRect& packed) noexcept;
        static void blit(ImTextureData& texture, const GlyphBitmap& glyph, const AtlasRect& packed) noexcept;
        void retire_page(std::size_t index) noexcept;
        void retire_pages() noexcept;
        void drop_pages() noexcept;

        ImDrawList* draw_list_{};
        ImVec2 origin_{0.f, 0.f};
        ImGuiContext* context_{};
        std::vector<Page> pages_;
        std::unordered_map<std::uint64_t, Slot> slots_;
        int last_frame_{-1};
        int max_atlas_size_{default_max_atlas_size};
        ImU32 color_{IM_COL32_BLACK};
    };

    // Draws a formula at the current ImGui cursor position (plus the surface origin, zero by default)
    // and reserves layout space for it. Works with whichever surface the formula's document owns.
    TEXRENDER_EXPORT void imgui_formula(const RenderHandle& formula);
} // namespace TeXRender

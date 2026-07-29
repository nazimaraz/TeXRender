#include "texrender/imgui_surface.hpp"
#include <algorithm>
#include <cmath>
#include <vector>
#include <imgui_internal.h>
#include "backend/glyph_bitmap_validation.hpp"
#include "render/glyph_cache.hpp"
#include "texrender/render_handle.hpp"

using namespace TeXRender;

namespace
{
    // Transparent gutter between packed glyphs so bilinear sampling cannot reach a neighbour.
    constexpr auto atlas_padding = 1;

    // Atlases retired by a reset or by a destroyed surface. They stay registered with ImGui until the
    // renderer backend reports the GPU texture as destroyed, which takes at least one frame.
    std::vector<ImTextureData*> g_retired;

    [[nodiscard]] bool finite_line(const float x1, const float y1, const float x2, const float y2, const float width)
    {
        return std::isfinite(x1) && std::isfinite(y1) && std::isfinite(x2) && std::isfinite(y2) && std::isfinite(width);
    }

    [[nodiscard]] bool finite_rect(const float x, const float y, const float w, const float h)
    {
        return std::isfinite(x) && std::isfinite(y) && std::isfinite(w) && std::isfinite(h) && w > 0.f && h > 0.f;
    }

    [[nodiscard]] float stroke_width(const float width)
    {
        return std::isfinite(width) ? std::max(width, 1.f) : 1.f;
    }

    [[nodiscard]] float corner_rounding(const float w, const float h, const float rx, const float ry)
    {
        return std::min({std::abs(rx), std::abs(ry), w * 0.5f, h * 0.5f});
    }

    // Largest atlas the renderer backend accepts, when it reports a limit at all.
    [[nodiscard]] int renderer_texture_limit() noexcept
    {
        if (!ImGui::GetCurrentContext())
            return 0;

        const auto& platform_io = ImGui::GetPlatformIO();
        if (platform_io.Renderer_TextureMaxWidth <= 0 || platform_io.Renderer_TextureMaxHeight <= 0)
            return 0;

        return std::min(platform_io.Renderer_TextureMaxWidth, platform_io.Renderer_TextureMaxHeight);
    }

    void warn_once_about_missing_texture_support()
    {
        static auto warned = false;
        if (warned)
            return;

        warned = true;
        detail::debug_log("ImGuiSurface: renderer backend does not set ImGuiBackendFlags_RendererHasTextures; "
                          "glyphs are skipped. Update the ImGui renderer backend to 1.92 or newer.");
    }
} // namespace

void TeXRender::imgui_collect_retired_textures() noexcept
{
    if (g_retired.empty())
        return;

    // Without a context ImGui no longer tracks the textures, so the memory is ours to release.
    if (!ImGui::GetCurrentContext())
    {
        for (auto* const texture : g_retired)
            IM_DELETE(texture);

        g_retired.clear();
        return;
    }

    for (auto it = g_retired.begin(); it != g_retired.end();)
    {
        auto* const texture = *it;
        if (texture->Status != ImTextureStatus_Destroyed)
        {
            ++it;
            continue;
        }

        ImGui::UnregisterUserTexture(texture);
        // ImGui only rebuilds this list on EndFrame, so drop the pointer now to keep it dangling-free
        // for a renderer backend that iterates the list before the next frame (e.g. during shutdown).
        ImGui::GetPlatformIO().Textures.find_erase(texture);
        IM_DELETE(texture);
        it = g_retired.erase(it);
    }
}

ImGuiSurface::ImGuiSurface() = default;

ImGuiSurface::ImGuiSurface(ImDrawList* const draw_list)
    : draw_list_{draw_list}
{}

ImGuiSurface::~ImGuiSurface() noexcept
{
    unload();
}

void ImGuiSurface::unload() noexcept
{
    retire_atlas();
    imgui_collect_retired_textures();
}

void ImGuiSurface::set_draw_list(ImDrawList* const draw_list) noexcept
{
    draw_list_ = draw_list;
}

ImDrawList* ImGuiSurface::draw_list() const noexcept
{
    return draw_list_;
}

void ImGuiSurface::set_origin(const ImVec2 origin) noexcept
{
    origin_ = origin;
}

ImVec2 ImGuiSurface::origin() const noexcept
{
    return origin_;
}

void ImGuiSurface::set_max_atlas_size(const int size)
{
    max_atlas_size_ = std::clamp(size, initial_atlas_size, largest_atlas_size);
}

int ImGuiSurface::max_atlas_size() const noexcept
{
    return max_atlas_size_;
}

int ImGuiSurface::atlas_size() const noexcept
{
    return atlas_ ? atlas_->Width : 0;
}

std::size_t ImGuiSurface::cached_glyphs() const noexcept
{
    return rects_.size();
}

bool ImGuiSurface::textures_supported() noexcept
{
    if (!ImGui::GetCurrentContext())
        return false;

    return (ImGui::GetIO().BackendFlags & ImGuiBackendFlags_RendererHasTextures) != 0;
}

ImDrawList* ImGuiSurface::target_list() const noexcept
{
    if (draw_list_)
        return draw_list_;

    if (!ImGui::GetCurrentContext())
        return nullptr;

    const auto* const window = ImGui::GetCurrentWindowRead();
    return window ? window->DrawList : nullptr;
}

ImVec2 ImGuiSurface::point(const float x, const float y) const noexcept
{
    return ImVec2{origin_.x + x, origin_.y + y};
}

void ImGuiSurface::set_color(const std::uint32_t argb)
{
    color_ = IM_COL32((argb >> 16) & 0xffu, (argb >> 8) & 0xffu, argb & 0xffu, (argb >> 24) & 0xffu);
}

void ImGuiSurface::draw_line(const float x1, const float y1, const float x2, const float y2, const float width)
{
    if (!finite_line(x1, y1, x2, y2, width))
        return;

    if (auto* const list = target_list())
        list->AddLine(point(x1, y1), point(x2, y2), color_, stroke_width(width));
}

void ImGuiSurface::draw_rect(const float x, const float y, const float w, const float h, const float width)
{
    if (!finite_rect(x, y, w, h))
        return;

    if (auto* const list = target_list())
        list->AddRect(point(x, y), point(x + w, y + h), color_, 0.f, stroke_width(width));
}

void ImGuiSurface::fill_rect(const float x, const float y, const float w, const float h)
{
    if (!finite_rect(x, y, w, h))
        return;

    if (auto* const list = target_list())
        list->AddRectFilled(point(x, y), point(x + w, y + h), color_);
}

void ImGuiSurface::draw_round_rect(const float x, const float y, const float w, const float h, const float rx, const float ry,
    const float width)
{
    if (!finite_rect(x, y, w, h) || !std::isfinite(rx) || !std::isfinite(ry))
        return;

    if (auto* const list = target_list())
        list->AddRect(point(x, y), point(x + w, y + h), color_, corner_rounding(w, h, rx, ry), stroke_width(width));
}

void ImGuiSurface::fill_round_rect(const float x, const float y, const float w, const float h, const float rx, const float ry)
{
    if (!finite_rect(x, y, w, h) || !std::isfinite(rx) || !std::isfinite(ry))
        return;

    if (auto* const list = target_list())
        list->AddRectFilled(point(x, y), point(x + w, y + h), color_, corner_rounding(w, h, rx, ry));
}

bool ImGuiSurface::create_atlas(const int size)
{
    auto* const texture = IM_NEW(ImTextureData)();
    texture->Create(ImTextureFormat_RGBA32, size, size);
    if (!texture->Pixels)
    {
        IM_DELETE(texture);
        detail::debug_log("ImGuiSurface: failed to allocate the glyph atlas");
        return false;
    }

    ImGui::RegisterUserTexture(texture);
    atlas_ = texture;
    rects_.clear();
    shelf_x_ = 0;
    shelf_y_ = 0;
    shelf_height_ = 0;
    return true;
}

void ImGuiSurface::retire_atlas() noexcept
{
    if (!atlas_)
        return;

    // Draw commands recorded this frame still reference the atlas, so ImGui destroys it next frame.
    atlas_->WantDestroyNextFrame = true;
    g_retired.push_back(atlas_);
    atlas_ = nullptr;
    rects_.clear();
    shelf_x_ = 0;
    shelf_y_ = 0;
    shelf_height_ = 0;
}

bool ImGuiSurface::pack(const int w, const int h, AtlasRect& packed) noexcept
{
    if (!atlas_)
        return false;

    if (shelf_x_ + w > atlas_->Width)
    {
        shelf_x_ = 0;
        shelf_y_ += shelf_height_ + atlas_padding;
        shelf_height_ = 0;
    }

    if (shelf_x_ + w > atlas_->Width || shelf_y_ + h > atlas_->Height)
        return false;

    packed = AtlasRect{shelf_x_, shelf_y_, w, h};
    shelf_x_ += w + atlas_padding;
    shelf_height_ = std::max(shelf_height_, h);
    return true;
}

void ImGuiSurface::blit(const GlyphBitmap& glyph, const AtlasRect& packed) const noexcept
{
    for (auto row = 0; row < glyph.height; ++row)
    {
        auto* const destination = static_cast<unsigned char*>(atlas_->GetPixelsAt(packed.x, packed.y + row));
        const auto* const source = glyph.buffer + static_cast<std::size_t>(row) * static_cast<std::size_t>(glyph.pitch);
        for (auto col = 0; col < glyph.width; ++col)
        {
            const auto pixel = static_cast<std::size_t>(col) * 4u;
            destination[pixel + 0] = 255;
            destination[pixel + 1] = 255;
            destination[pixel + 2] = 255;
            destination[pixel + 3] = source[col];
        }
    }
}

const ImGuiSurface::AtlasRect* ImGuiSurface::rect_for(const GlyphBitmap& glyph)
{
    imgui_collect_retired_textures();

    if (const auto found = rects_.find(glyph.key); found != rects_.end())
        return &found->second;

    const auto renderer_limit = renderer_texture_limit();
    const auto limit = renderer_limit > 0 ? std::min(max_atlas_size_, renderer_limit) : max_atlas_size_;
    const auto needed = std::max(glyph.width, glyph.height) + atlas_padding;
    if (needed > limit)
    {
        detail::debug_log("ImGuiSurface: glyph is larger than the maximum glyph atlas size");
        return nullptr;
    }

    if (!atlas_)
    {
        auto size = std::min(initial_atlas_size, limit);
        while (size < needed)
            size *= 2;

        if (!create_atlas(std::min(size, limit)))
            return nullptr;
    }

    auto packed = AtlasRect{};
    if (!pack(glyph.width, glyph.height, packed))
    {
        // Full: grow the atlas, or start over at the same size once the limit is reached.
        const auto size = std::min(limit, std::max(atlas_->Width * 2, needed));
        retire_atlas();
        if (!create_atlas(size) || !pack(glyph.width, glyph.height, packed))
            return nullptr;
    }

    blit(glyph, packed);
    ImTextureDataQueueUpload(atlas_, packed.x, packed.y, packed.w, packed.h);
    return &rects_.emplace(glyph.key, packed).first->second;
}

void ImGuiSurface::draw_glyph(const GlyphBitmap& glyph, const float pen_x, const float pen_y, const float angle)
{
    if (!detail::valid_cacheable_glyph_bitmap_for_rgba(glyph) || !std::isfinite(pen_x) || !std::isfinite(pen_y) ||
        !std::isfinite(angle))
        return;

    auto* const list = target_list();
    if (!list)
        return;

    if (!textures_supported())
    {
        warn_once_about_missing_texture_support();
        return;
    }

    const auto* const packed = rect_for(glyph);
    if (!packed)
        return;

    const auto atlas_width = static_cast<float>(atlas_->Width);
    const auto atlas_height = static_cast<float>(atlas_->Height);
    const auto uv_min = ImVec2{static_cast<float>(packed->x) / atlas_width, static_cast<float>(packed->y) / atlas_height};
    const auto uv_max =
        ImVec2{static_cast<float>(packed->x + packed->w) / atlas_width, static_cast<float>(packed->y + packed->h) / atlas_height};
    const auto left = static_cast<float>(glyph.left);
    const auto top = static_cast<float>(glyph.top);
    const auto w = static_cast<float>(glyph.width);
    const auto h = static_cast<float>(glyph.height);

    if (angle == 0.f)
    {
        const auto min = point(pen_x + left, pen_y - top);
        list->AddImage(atlas_->GetTexRef(), min, ImVec2{min.x + w, min.y + h}, uv_min, uv_max, color_);
        return;
    }

    // Rotate the quad around the pen, matching the other backends' clockwise screen-space rotation.
    const auto sin_angle = std::sin(angle);
    const auto cos_angle = std::cos(angle);
    const auto pivot = point(pen_x, pen_y);
    const auto corner = [&](const float dx, const float dy) {
        return ImVec2{pivot.x + dx * cos_angle - dy * sin_angle, pivot.y + dx * sin_angle + dy * cos_angle};
    };
    list->AddImageQuad(atlas_->GetTexRef(), corner(left, -top), corner(left + w, -top), corner(left + w, -top + h),
        corner(left, -top + h), uv_min, ImVec2{uv_max.x, uv_min.y}, uv_max, ImVec2{uv_min.x, uv_max.y}, color_);
}

void TeXRender::imgui_formula(ImGuiSurface& surface, const RenderHandle& formula)
{
    const auto size = ImVec2{static_cast<float>(formula.width()), static_cast<float>(formula.height())};
    const auto previous = surface.origin();
    surface.set_origin(ImGui::GetCursorScreenPos());
    formula.draw(0, 0);
    surface.set_origin(previous);
    ImGui::Dummy(size);
}

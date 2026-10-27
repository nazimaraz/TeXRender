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
    constexpr auto max_glyph_scale = 4.f;

    // An atlas page retired by a reset, an eviction or a destroyed surface. It stays registered with its
    // context until that context's renderer backend reports the GPU texture as destroyed.
    struct RetiredTexture
    {
        ImTextureData* texture{};
        ImGuiContext* context{};
    };

    // ImGui runs on one thread, and these are only touched from it.
    std::vector<RetiredTexture> g_retired;
    std::vector<ImGuiSurface*> g_surfaces;
    std::vector<ImGuiContext*> g_hooked_contexts;

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
        const auto& platform_io = ImGui::GetPlatformIO();
        if (platform_io.Renderer_TextureMaxWidth <= 0 || platform_io.Renderer_TextureMaxHeight <= 0)
            return 0;

        return std::min(platform_io.Renderer_TextureMaxWidth, platform_io.Renderer_TextureMaxHeight);
    }

    // Removes a texture from the current context and frees it. ImGui only rebuilds the platform texture
    // list on EndFrame, so the pointer is dropped from it too, keeping it valid for a renderer backend
    // that iterates the list before the next frame (e.g. during shutdown).
    void release_texture(ImTextureData* const texture) noexcept
    {
        ImGui::UnregisterUserTexture(texture);
        ImGui::GetPlatformIO().Textures.find_erase(texture);
        IM_DELETE(texture);
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
    const auto* const context = ImGui::GetCurrentContext();
    if (!context)
        return;

    for (auto it = g_retired.begin(); it != g_retired.end();)
    {
        if (it->context != context || it->texture->Status != ImTextureStatus_Destroyed)
        {
            ++it;
            continue;
        }

        release_texture(it->texture);
        it = g_retired.erase(it);
    }
}

void ImGuiSurface::on_context_shutdown(ImGuiContext* const context, ImGuiContextHook* /*hook*/)
{
    // DestroyContext() makes the dying context current while this runs, so release_texture() applies to it.
    for (auto it = g_retired.begin(); it != g_retired.end();)
    {
        if (it->context != context)
        {
            ++it;
            continue;
        }

        if (it->texture->Status != ImTextureStatus_Destroyed)
            detail::debug_log("ImGuiSurface: glyph atlas outlived its renderer backend; shut the backend down first");

        release_texture(it->texture);
        it = g_retired.erase(it);
    }

    for (auto* const surface : g_surfaces)
    {
        if (surface->context_ == context)
            surface->drop_pages();
    }

    g_hooked_contexts.erase(std::remove(g_hooked_contexts.begin(), g_hooked_contexts.end(), context), g_hooked_contexts.end());
}

ImGuiSurface::ImGuiSurface()
    : ImGuiSurface{nullptr}
{}

ImGuiSurface::ImGuiSurface(ImDrawList* const draw_list)
    : draw_list_{draw_list}
{
    g_surfaces.push_back(this);
}

ImGuiSurface::~ImGuiSurface() noexcept
{
    unload();
    g_surfaces.erase(std::remove(g_surfaces.begin(), g_surfaces.end(), this), g_surfaces.end());
}

void ImGuiSurface::unload() noexcept
{
    retire_pages();
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
    return pages_.empty() ? 0 : pages_.back().texture->Width;
}

std::size_t ImGuiSurface::atlas_pages() const noexcept
{
    return pages_.size();
}

std::size_t ImGuiSurface::cached_glyphs() const noexcept
{
    return slots_.size();
}

bool ImGuiSurface::textures_supported() noexcept
{
    if (!ImGui::GetCurrentContext())
        return false;

    return (ImGui::GetIO().BackendFlags & ImGuiBackendFlags_RendererHasTextures) != 0;
}

float ImGuiSurface::glyph_scale() const noexcept
{
    if (!ImGui::GetCurrentContext())
        return 1.f;

    const auto scale = ImGui::GetIO().DisplayFramebufferScale.y;
    return std::isfinite(scale) ? std::clamp(scale, 1.f, max_glyph_scale) : 1.f;
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

ImGuiSurface::Page* ImGuiSurface::add_page(const int size, const int frame)
{
    auto* const texture = IM_NEW(ImTextureData)();
    texture->Create(ImTextureFormat_RGBA32, size, size);
    if (!texture->Pixels)
    {
        IM_DELETE(texture);
        detail::debug_log("ImGuiSurface: failed to allocate a glyph atlas page");
        return nullptr;
    }

    ImGui::RegisterUserTexture(texture);
    auto& page = pages_.emplace_back();
    page.texture = texture;
    page.last_used_frame = frame;
    return &page;
}

ImGuiSurface::Page* ImGuiSurface::page_of(const ImTextureData* const texture) noexcept
{
    const auto found = std::find_if(pages_.begin(), pages_.end(), [&](const Page& page) {
        return page.texture == texture;
    });
    return found != pages_.end() ? &*found : nullptr;
}

void ImGuiSurface::retire_page(const std::size_t index) noexcept
{
    auto* const texture = pages_[index].texture;
    // Draw commands recorded this frame may still reference the page, so ImGui destroys it next frame.
    texture->WantDestroyNextFrame = true;
    try
    {
        g_retired.push_back(RetiredTexture{texture, context_});
    } catch (...)
    {
        // Out of memory: the page stays registered and the backend still destroys its GPU texture;
        // only the small ImTextureData allocation is lost.
        detail::debug_log("ImGuiSurface: could not track a retired glyph atlas page");
    }

    pages_.erase(pages_.begin() + static_cast<std::ptrdiff_t>(index));
    for (auto it = slots_.begin(); it != slots_.end();)
        it = it->second.texture == texture ? slots_.erase(it) : std::next(it);
}

void ImGuiSurface::retire_pages() noexcept
{
    while (!pages_.empty())
        retire_page(pages_.size() - 1);
}

void ImGuiSurface::drop_pages() noexcept
{
    for (const auto& page : pages_)
        release_texture(page.texture);

    pages_.clear();
    slots_.clear();
    context_ = nullptr;
    last_frame_ = -1;
}

void ImGuiSurface::begin_frame(const int frame)
{
    imgui_collect_retired_textures();

    // Release pages that stopped being drawn from, keeping the newest so steady content never thrashes.
    for (auto index = pages_.size(); index-- > 0;)
    {
        if (pages_.size() > 1 && frame - pages_[index].last_used_frame > stale_page_frames)
            retire_page(index);
    }
}

bool ImGuiSurface::pack(Page& page, const int w, const int h, AtlasRect& packed) noexcept
{
    const auto width = page.texture->Width;
    const auto height = page.texture->Height;
    auto shelf_x = page.shelf_x;
    auto shelf_y = page.shelf_y;
    auto shelf_height = page.shelf_height;
    if (shelf_x + w > width)
    {
        shelf_x = 0;
        shelf_y += shelf_height + atlas_padding;
        shelf_height = 0;
    }

    if (shelf_x + w > width || shelf_y + h > height)
        return false;

    packed = AtlasRect{shelf_x, shelf_y, w, h};
    page.shelf_x = shelf_x + w + atlas_padding;
    page.shelf_y = shelf_y;
    page.shelf_height = std::max(shelf_height, h);
    return true;
}

void ImGuiSurface::blit(ImTextureData& texture, const GlyphBitmap& glyph, const AtlasRect& packed) noexcept
{
    for (auto row = 0; row < glyph.height; ++row)
    {
        auto* const destination = static_cast<unsigned char*>(texture.GetPixelsAt(packed.x, packed.y + row));
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

const ImGuiSurface::Slot* ImGuiSurface::slot_for(const GlyphBitmap& glyph)
{
    auto* const context = ImGui::GetCurrentContext();
    if (context_ != context)
    {
        // Pages belong to the context they were registered with; start over in the new one.
        retire_pages();
        context_ = context;
        last_frame_ = -1;
        if (std::find(g_hooked_contexts.begin(), g_hooked_contexts.end(), context) == g_hooked_contexts.end())
        {
            auto hook = ImGuiContextHook{};
            hook.Type = ImGuiContextHookType_Shutdown;
            hook.Callback = &ImGuiSurface::on_context_shutdown;
            ImGui::AddContextHook(context, &hook);
            g_hooked_contexts.push_back(context);
        }
    }

    const auto frame = ImGui::GetFrameCount();
    if (frame != last_frame_)
    {
        last_frame_ = frame;
        begin_frame(frame);
    }

    if (const auto found = slots_.find(glyph.key); found != slots_.end())
    {
        if (auto* const page = page_of(found->second.texture))
            page->last_used_frame = frame;

        return &found->second;
    }

    const auto renderer_limit = renderer_texture_limit();
    const auto limit = renderer_limit > 0 ? std::min(max_atlas_size_, renderer_limit) : max_atlas_size_;
    const auto needed = std::max(glyph.width, glyph.height) + atlas_padding;
    if (needed > limit)
    {
        detail::debug_log("ImGuiSurface: glyph is larger than the maximum glyph atlas size");
        return nullptr;
    }

    auto packed = AtlasRect{};
    Page* target = nullptr;
    for (auto index = pages_.size(); index-- > 0 && !target;)
    {
        if (pack(pages_[index], glyph.width, glyph.height, packed))
            target = &pages_[index];
    }

    if (!target)
    {
        auto size = limit;
        if (pages_.empty())
        {
            size = std::min(initial_atlas_size, limit);
            while (size < needed)
                size *= 2;
        }
        else if (pages_.size() == 1 && pages_.front().texture->Width < limit)
        {
            // While there is a single page, grow it; its glyphs are re-packed into the larger one on demand.
            size = std::max(pages_.front().texture->Width * 2, needed);
            retire_page(0);
        }

        // Past the size limit, overflow goes to additional pages, so a large working set settles
        // instead of resetting the atlas every frame.
        target = add_page(std::min(size, limit), frame);
        if (!target || !pack(*target, glyph.width, glyph.height, packed))
            return nullptr;
    }

    target->last_used_frame = frame;
    blit(*target->texture, glyph, packed);
    ImTextureDataQueueUpload(target->texture, packed.x, packed.y, packed.w, packed.h);
    return &slots_.insert_or_assign(glyph.key, Slot{target->texture, packed}).first->second;
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

    const auto* const slot = slot_for(glyph);
    if (!slot)
        return;

    const auto& packed = slot->rect;
    const auto atlas_width = static_cast<float>(slot->texture->Width);
    const auto atlas_height = static_cast<float>(slot->texture->Height);
    const auto uv_min = ImVec2{static_cast<float>(packed.x) / atlas_width, static_cast<float>(packed.y) / atlas_height};
    const auto uv_max =
        ImVec2{static_cast<float>(packed.x + packed.w) / atlas_width, static_cast<float>(packed.y + packed.h) / atlas_height};

    // Bitmaps arrive rasterized at glyph_scale(); draw them back at layout size.
    const auto scale = glyph_scale();
    const auto left = static_cast<float>(glyph.left) / scale;
    const auto top = static_cast<float>(glyph.top) / scale;
    const auto w = static_cast<float>(glyph.width) / scale;
    const auto h = static_cast<float>(glyph.height) / scale;
    const auto texture = slot->texture->GetTexRef();

    if (angle == 0.f)
    {
        const auto min = point(pen_x + left, pen_y - top);
        list->AddImage(texture, min, ImVec2{min.x + w, min.y + h}, uv_min, uv_max, color_);
        return;
    }

    // Rotate the quad around the pen, matching the other backends' clockwise screen-space rotation.
    const auto sin_angle = std::sin(angle);
    const auto cos_angle = std::cos(angle);
    const auto pivot = point(pen_x, pen_y);
    const auto corner = [&](const float dx, const float dy) {
        return ImVec2{pivot.x + dx * cos_angle - dy * sin_angle, pivot.y + dx * sin_angle + dy * cos_angle};
    };
    list->AddImageQuad(texture, corner(left, -top), corner(left + w, -top), corner(left + w, -top + h), corner(left, -top + h),
        uv_min, ImVec2{uv_max.x, uv_min.y}, uv_max, ImVec2{uv_min.x, uv_max.y}, color_);
}

void TeXRender::imgui_formula(const RenderHandle& formula)
{
    const auto position = ImGui::GetCursorScreenPos();
    formula.draw(position.x, position.y);
    ImGui::Dummy(ImVec2{static_cast<float>(formula.width()), static_cast<float>(formula.height())});
}

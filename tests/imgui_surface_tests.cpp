#include <algorithm>
#include <cmath>
#include <cstdio>
#include <memory>
#include <vector>
#include <imgui.h>
#include <texrender/imgui_surface.hpp>
#include <texrender/latex_document.hpp>
#include "test_helpers.hpp"
#ifndef TEXRENDER_RES_DIR
#   define TEXRENDER_RES_DIR "res"
#endif

using namespace TeXRender;
using namespace TeXRender::Test;

namespace
{
    // Stands in for a renderer backend that advertises ImGuiBackendFlags_RendererHasTextures: it honours
    // the create/update/destroy requests the current context queues, without needing a window or a GPU.
    class FakeRenderer
    {
    public:
        void honor_texture_requests()
        {
            for (auto* const texture : ImGui::GetPlatformIO().Textures)
            {
                if (texture->Status == ImTextureStatus_WantCreate)
                {
                    texture->SetTexID(static_cast<ImTextureID>(++next_id_));
                    texture->SetStatus(ImTextureStatus_OK);
                    ++created_;
                }
                else if (texture->Status == ImTextureStatus_WantUpdates)
                {
                    texture->SetStatus(ImTextureStatus_OK);
                    ++updated_;
                }
                else if (texture->Status == ImTextureStatus_WantDestroy)
                {
                    texture->SetTexID(ImTextureID_Invalid);
                    texture->SetStatus(ImTextureStatus_Destroyed);
                    ++destroyed_;
                }
            }
        }

        [[nodiscard]] int created() const noexcept
        {
            return created_;
        }

        [[nodiscard]] int updated() const noexcept
        {
            return updated_;
        }

        [[nodiscard]] int destroyed() const noexcept
        {
            return destroyed_;
        }

    private:
        unsigned long long next_id_{0};
        int created_{0};
        int updated_{0};
        int destroyed_{0};
    };

    struct Bounds
    {
        float min_x{0.f};
        float min_y{0.f};
        float max_x{0.f};
        float max_y{0.f};

        [[nodiscard]] float width() const noexcept
        {
            return max_x - min_x;
        }
    };

    [[nodiscard]] ImGuiContext* create_context()
    {
        // CreateContext() restores whichever context was current, so switch to the new one explicitly.
        auto* const context = ImGui::CreateContext();
        ImGui::SetCurrentContext(context);
        auto& io = ImGui::GetIO();
        io.DisplaySize = ImVec2{800.f, 600.f};
        io.DeltaTime = 1.f / 60.f;
        io.IniFilename = nullptr;
        io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
        io.Fonts->AddFontDefault();
        return context;
    }

    // Glyph atlases are the registered textures that no font atlas owns.
    [[nodiscard]] std::vector<ImTextureData*> find_glyph_atlases()
    {
        const auto& font_textures = ImGui::GetIO().Fonts->TexList;
        auto found = std::vector<ImTextureData*>{};
        for (auto* const texture : ImGui::GetPlatformIO().Textures)
        {
            if (std::find(font_textures.begin(), font_textures.end(), texture) == font_textures.end())
                found.push_back(texture);
        }

        return found;
    }

    [[nodiscard]] ImTextureData* find_glyph_atlas()
    {
        const auto found = find_glyph_atlases();
        return found.size() == 1 ? found.front() : nullptr;
    }

    [[nodiscard]] bool has_opaque_pixels(const ImTextureData& texture)
    {
        if (!texture.Pixels)
            return false;

        const auto bytes = static_cast<std::size_t>(texture.GetSizeInBytes());
        for (auto offset = std::size_t{3}; offset < bytes; offset += 4u)
        {
            if (texture.Pixels[offset] != 0)
                return true;
        }

        return false;
    }

    void begin_frame()
    {
        ImGui::NewFrame();
        ImGui::SetNextWindowPos(ImVec2{0.f, 0.f});
        ImGui::SetNextWindowSize(ImVec2{780.f, 580.f});
        ImGui::Begin("formulas");
    }

    void end_frame(FakeRenderer& renderer)
    {
        ImGui::End();
        ImGui::Render();
        renderer.honor_texture_requests();
    }

    void run_frames(FakeRenderer& renderer, const RenderHandle& formula, const int count)
    {
        for (auto frame = 0; frame < count; ++frame)
        {
            begin_frame();
            imgui_formula(formula);
            end_frame(renderer);
        }
    }

    // Bounds of the vertices a draw adds to the current window's draw list.
    template <typename Draw>
    [[nodiscard]] Bounds drawn_bounds(Draw&& draw)
    {
        const auto& vertices = ImGui::GetWindowDrawList()->VtxBuffer;
        const auto first = vertices.Size;
        draw();
        auto bounds = Bounds{1e9f, 1e9f, -1e9f, -1e9f};
        for (auto index = first; index < vertices.Size; ++index)
        {
            bounds.min_x = std::min(bounds.min_x, vertices[index].pos.x);
            bounds.min_y = std::min(bounds.min_y, vertices[index].pos.y);
            bounds.max_x = std::max(bounds.max_x, vertices[index].pos.x);
            bounds.max_y = std::max(bounds.max_y, vertices[index].pos.y);
        }

        return bounds;
    }

    [[nodiscard]] bool near(const float value, const float expected, const float tolerance)
    {
        return std::abs(value - expected) <= tolerance;
    }

    [[nodiscard]] bool lifecycle_tests(FakeRenderer& renderer)
    {
        auto passed = true;
        auto destroyed_before_shutdown = 0;
        {
            // Render handles share ownership of the document state, so the surface only dies with them.
            auto owned = std::make_unique<ImGuiSurface>();
            auto& canvas = *owned;
            const auto document = LatexDocument{TEXRENDER_RES_DIR, std::move(owned)};
            const auto formula = document.render(R"(\frac{1}{2}+\sqrt{x^2+1})", 40.f, 0xff102030);
            passed &= run_case("formula has a size", formula.width() > 0 && formula.height() > 0);

            begin_frame();
            const auto cursor_before = ImGui::GetCursorPos();
            imgui_formula(formula);
            const auto cursor_after = ImGui::GetCursorPos();
            const auto* const draw_list = ImGui::GetWindowDrawList();
            const auto commands = draw_list->CmdBuffer.Size;
            const auto vertices = draw_list->VtxBuffer.Size;
            end_frame(renderer);

            passed &= run_case("layout space is reserved", cursor_after.y > cursor_before.y);
            passed &= run_case("glyphs are packed into the atlas", canvas.cached_glyphs() > 0);
            passed &= run_case("atlas is allocated", canvas.atlas_size() > 0 && canvas.atlas_pages() == 1);
            passed &= run_case("draw commands are recorded", commands > 0 && vertices > 0);
            passed &= run_case("backend was asked to create the atlas", renderer.created() >= 1);

            const auto* const atlas = find_glyph_atlas();
            passed &= run_case("atlas is registered with ImGui", atlas != nullptr);
            if (atlas)
            {
                passed &= run_case("atlas matches the reported size", atlas->Width == canvas.atlas_size());
                passed &= run_case("glyph coverage was written into the atlas", has_opaque_pixels(*atlas));
                passed &= run_case("atlas reached the backend", atlas->Status == ImTextureStatus_OK);
            }

            // Redrawing the same formula must reuse the packed glyphs instead of growing the atlas.
            const auto glyphs_after_first_frame = canvas.cached_glyphs();
            const auto updates_after_first_frame = renderer.updated();
            run_frames(renderer, formula, 1);
            passed &= run_case("cached glyphs are reused", canvas.cached_glyphs() == glyphs_after_first_frame);
            passed &= run_case("no further uploads for cached glyphs", renderer.updated() == updates_after_first_frame);

            // A new formula adds glyphs to the existing atlas and uploads only those.
            const auto other = document.render(R"(\int_0^1 e^{-x}\,dx)", 40.f, 0xff102030);
            run_frames(renderer, other, 1);
            passed &= run_case("new glyphs are added", canvas.cached_glyphs() > glyphs_after_first_frame);
            passed &= run_case("new glyphs are uploaded", renderer.updated() > updates_after_first_frame);
            passed &= run_case("atlas was not recreated", find_glyph_atlas() == atlas);

            // Glyphs too large for the current atlas grow it and retire the old one.
            const auto size_before_growth = canvas.atlas_size();
            const auto created_before_growth = renderer.created();
            const auto destroyed_before_growth = renderer.destroyed();
            const auto huge = document.render(R"(\sqrt{x+1})", 900.f, 0xff102030);
            run_frames(renderer, huge, 1);
            passed &= run_case("atlas grew for oversized glyphs", canvas.atlas_size() > size_before_growth);
            passed &= run_case("a larger atlas was created", renderer.created() > created_before_growth);
            passed &= run_case("a single page remains after growth", canvas.atlas_pages() == 1);

            // A further frame lets the backend release the retired atlas, leaving only the current one.
            run_frames(renderer, huge, 1);
            imgui_collect_retired_textures();
            passed &= run_case("retired atlas was destroyed", renderer.destroyed() > destroyed_before_growth);
            passed &= run_case("only the current atlas is registered", find_glyph_atlases().size() == 1);
            destroyed_before_shutdown = renderer.destroyed();
        }

        // The surface is gone: the atlas stays registered until the backend reports it destroyed.
        passed &= run_case("atlas is queued for destruction", find_glyph_atlases().size() == 1);
        begin_frame();
        end_frame(renderer);
        passed &= run_case("backend destroyed the atlas", renderer.destroyed() > destroyed_before_shutdown);
        imgui_collect_retired_textures();
        passed &= run_case("atlas is unregistered", find_glyph_atlases().empty());
        return passed;
    }

    [[nodiscard]] bool placement_tests(FakeRenderer& renderer)
    {
        auto passed = true;
        const auto document = LatexDocument{TEXRENDER_RES_DIR, std::make_unique<ImGuiSurface>()};
        const auto formula = document.render(R"(x^2)", 40.f, 0xff102030);
        run_frames(renderer, formula, 1);

        begin_frame();
        const auto whole = drawn_bounds([&] {
            formula.draw(100, 100);
        });
        const auto fractional = drawn_bounds([&] {
            formula.draw(100.25f, 100.5f);
        });
        const auto cursor = ImGui::GetCursorScreenPos();
        const auto at_cursor = drawn_bounds([&] {
            imgui_formula(formula);
        });
        const auto direct = drawn_bounds([&] {
            formula.draw(cursor.x, cursor.y);
        });
        end_frame(renderer);

        passed &= run_case("float draw keeps sub-pixel x", near(fractional.min_x - whole.min_x, 0.25f, 1e-3f));
        passed &= run_case("float draw keeps sub-pixel y", near(fractional.min_y - whole.min_y, 0.5f, 1e-3f));
        passed &= run_case("imgui_formula draws at the cursor",
            near(at_cursor.min_x, direct.min_x, 1e-3f) && near(at_cursor.min_y, direct.min_y, 1e-3f));
        return passed;
    }

    [[nodiscard]] bool high_dpi_tests(FakeRenderer& renderer)
    {
        auto passed = true;
        auto owned = std::make_unique<ImGuiSurface>();
        auto& canvas = *owned;
        const auto document = LatexDocument{TEXRENDER_RES_DIR, std::move(owned)};
        const auto formula = document.render(R"(\mathrm{Wave})", 40.f, 0xff102030);

        begin_frame();
        const auto standard = drawn_bounds([&] {
            formula.draw(20, 20);
        });
        end_frame(renderer);
        const auto standard_glyphs = canvas.cached_glyphs();

        ImGui::GetIO().DisplayFramebufferScale = ImVec2{2.f, 2.f};
        passed &= run_case("glyph scale follows the framebuffer scale", canvas.glyph_scale() == 2.f);
        begin_frame();
        const auto retina = drawn_bounds([&] {
            formula.draw(20, 20);
        });
        end_frame(renderer);
        ImGui::GetIO().DisplayFramebufferScale = ImVec2{1.f, 1.f};

        passed &= run_case("glyphs are rasterized again at 2x", canvas.cached_glyphs() > standard_glyphs);
        passed &= run_case("2x glyphs keep the layout size", near(retina.width(), standard.width(), standard.width() * 0.06f));
        return passed;
    }

    [[nodiscard]] bool paging_tests(FakeRenderer& renderer)
    {
        auto passed = true;
        auto owned = std::make_unique<ImGuiSurface>();
        auto& canvas = *owned;
        canvas.set_max_atlas_size(ImGuiSurface::initial_atlas_size);
        const auto document = LatexDocument{TEXRENDER_RES_DIR, std::move(owned)};
        // At this size only a couple of glyphs fit a 512x512 page, so the formula spans several pages.
        const auto wide = document.render(R"(\mathrm{abcdefgh})", 400.f, 0xff102030);
        run_frames(renderer, wide, 2);
        const auto pages = canvas.atlas_pages();
        const auto created = renderer.created();
        const auto updated = renderer.updated();
        passed &= run_case("overflow spills into extra pages", pages > 1);
        passed &= run_case("pages stay at the size limit", canvas.atlas_size() == ImGuiSurface::initial_atlas_size);

        run_frames(renderer, wide, 5);
        passed &= run_case("a working set larger than one page does not thrash",
            renderer.created() == created && renderer.updated() == updated && canvas.atlas_pages() == pages);

        // Once the large formula stops being drawn, its pages age out and are released.
        const auto destroyed = renderer.destroyed();
        const auto small = document.render(R"(z)", 40.f, 0xff102030);
        run_frames(renderer, small, ImGuiSurface::stale_page_frames + 3);
        passed &= run_case("stale pages are released", canvas.atlas_pages() < pages && renderer.destroyed() > destroyed);
        passed &= run_case("glyphs in use survive eviction", canvas.cached_glyphs() > 0 && canvas.atlas_pages() >= 1);
        return passed;
    }

    [[nodiscard]] bool context_tests(ImGuiContext* const main_context, FakeRenderer& renderer)
    {
        auto passed = true;
        auto owned = std::make_unique<ImGuiSurface>();
        auto& canvas = *owned;
        const auto document = LatexDocument{TEXRENDER_RES_DIR, std::move(owned)};
        const auto formula = document.render(R"(y=mx+b)", 40.f, 0xff102030);
        run_frames(renderer, formula, 1);
        const auto main_atlases = find_glyph_atlases().size();

        // Moving to another context starts new pages there and retires the ones in the first context.
        auto* const other_context = create_context();
        auto other_renderer = FakeRenderer{};
        run_frames(other_renderer, formula, 1);
        passed &=
            run_case("pages are created in the second context", canvas.atlas_pages() == 1 && find_glyph_atlases().size() == 1);

        ImGui::SetCurrentContext(main_context);
        passed &= run_case("first context still holds the retired page", find_glyph_atlases().size() == main_atlases);
        begin_frame();
        end_frame(renderer);
        imgui_collect_retired_textures();
        passed &= run_case("retired page is released in its own context", find_glyph_atlases().empty());

        // Destroying the second context releases the live pages it owned.
        ImGui::DestroyContext(other_context);
        passed &= run_case("destroying a context drops its pages", canvas.atlas_pages() == 0 && canvas.cached_glyphs() == 0);
        passed &= run_case("the main context stays current", ImGui::GetCurrentContext() == main_context);

        // The surface keeps working once it is back in the main context.
        run_frames(renderer, formula, 1);
        passed &= run_case("surface recovers in the main context", canvas.atlas_pages() == 1 && canvas.cached_glyphs() > 0);
        return passed;
    }
} // namespace

int main()
{
    IMGUI_CHECKVERSION();
    auto* const context = create_context();
    auto renderer = FakeRenderer{};
    auto passed = true;
    passed &= run_case("textures supported once the backend flag is set", ImGuiSurface::textures_supported());
    passed &= lifecycle_tests(renderer);
    passed &= placement_tests(renderer);
    passed &= high_dpi_tests(renderer);
    passed &= paging_tests(renderer);
    passed &= context_tests(context, renderer);

    ImGui::DestroyContext(context);
    if (!passed)
    {
        std::fprintf(stderr, "imgui surface tests failed\n");
        return 1;
    }

    std::printf("imgui surface tests passed\n");
    return 0;
}

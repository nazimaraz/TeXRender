#include <algorithm>
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
    // the create/update/destroy requests ImGui queues, without needing a window or a GPU.
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
        ImGui::SetNextWindowSize(ImVec2{640.f, 480.f});
        ImGui::Begin("formulas");
    }

    void end_frame(FakeRenderer& renderer)
    {
        ImGui::End();
        ImGui::Render();
        renderer.honor_texture_requests();
    }
} // namespace

int main()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.DisplaySize = ImVec2{800.f, 600.f};
    io.DeltaTime = 1.f / 60.f;
    io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
    io.Fonts->AddFontDefault();

    auto renderer = FakeRenderer{};
    auto passed = true;
    passed &= run_case("textures supported once the backend flag is set", ImGuiSurface::textures_supported());

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
        imgui_formula(canvas, formula);
        const auto cursor_after = ImGui::GetCursorPos();
        const auto* const draw_list = ImGui::GetWindowDrawList();
        const auto commands = draw_list->CmdBuffer.Size;
        const auto vertices = draw_list->VtxBuffer.Size;
        end_frame(renderer);

        passed &= run_case("layout space is reserved", cursor_after.y > cursor_before.y);
        passed &= run_case("glyphs are packed into the atlas", canvas.cached_glyphs() > 0);
        passed &= run_case("atlas is allocated", canvas.atlas_size() > 0);
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
        begin_frame();
        imgui_formula(canvas, formula);
        end_frame(renderer);
        passed &= run_case("cached glyphs are reused", canvas.cached_glyphs() == glyphs_after_first_frame);
        passed &= run_case("no further uploads for cached glyphs", renderer.updated() == updates_after_first_frame);

        // A new formula adds glyphs to the existing atlas and uploads only those.
        const auto other = document.render(R"(\int_0^1 e^{-x}\,dx)", 40.f, 0xff102030);
        begin_frame();
        imgui_formula(canvas, other);
        end_frame(renderer);
        passed &= run_case("new glyphs are added", canvas.cached_glyphs() > glyphs_after_first_frame);
        passed &= run_case("new glyphs are uploaded", renderer.updated() > updates_after_first_frame);
        passed &= run_case("atlas was not recreated", find_glyph_atlas() == atlas);

        // Glyphs too large for the current atlas force a bigger one and retire the old atlas.
        const auto size_before_growth = canvas.atlas_size();
        const auto created_before_growth = renderer.created();
        const auto destroyed_before_growth = renderer.destroyed();
        const auto huge = document.render(R"(\sqrt{x+1})", 900.f, 0xff102030);
        begin_frame();
        imgui_formula(canvas, huge);
        end_frame(renderer);
        passed &= run_case("atlas grew for oversized glyphs", canvas.atlas_size() > size_before_growth);
        passed &= run_case("a larger atlas was created", renderer.created() > created_before_growth);
        passed &= run_case("glyphs are packed after growth", canvas.cached_glyphs() > 0);

        // A further frame lets the backend release the retired atlas, leaving only the current one.
        begin_frame();
        imgui_formula(canvas, huge);
        end_frame(renderer);
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

    ImGui::DestroyContext();
    if (!passed)
    {
        std::fprintf(stderr, "imgui surface tests failed\n");
        return 1;
    }

    std::printf("imgui surface tests passed\n");
    return 0;
}

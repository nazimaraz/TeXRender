#include <cstdio>
#include <memory>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#include <SDL.h>
#include <texrender/imgui_surface.hpp>
#include <texrender/latex_document.hpp>
#ifndef TEXRENDER_RES_DIR
#   define TEXRENDER_RES_DIR "res"
#endif

int main(int, char**)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    auto* const window =
        SDL_CreateWindow("texrender ImGui", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 760, 460, SDL_WINDOW_SHOWN);
    if (!window)
    {
        std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    auto* const renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer)
    {
        std::fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsLight();
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    {
        // The surface draws into the current window's draw list, so nothing else needs wiring up.
        auto owned = std::make_unique<TeXRender::ImGuiSurface>();
        auto& canvas = *owned;
        const auto document = TeXRender::LatexDocument{TEXRENDER_RES_DIR, std::move(owned)};
        const auto identity = document.render(R"(e^{i\pi} + 1 = 0)", 48.f, 0xff101820);
        const auto basel = document.render(R"(\sum_{n=1}^{\infty}\frac{1}{n^2}=\frac{\pi^2}{6})", 40.f, 0xff1d4ed8);
        auto running = true;
        while (running)
        {
            auto event = SDL_Event{};
            while (SDL_PollEvent(&event) != 0)
            {
                ImGui_ImplSDL2_ProcessEvent(&event);
                if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE))
                    running = false;
            }

            ImGui_ImplSDLRenderer2_NewFrame();
            ImGui_ImplSDL2_NewFrame();
            ImGui::NewFrame();

            ImGui::SetNextWindowSize(ImVec2{620.f, 380.f}, ImGuiCond_FirstUseEver);
            if (ImGui::Begin("Formulas"))
            {
                ImGui::TextUnformatted("Euler's identity");
                TeXRender::imgui_formula(canvas, identity);
                ImGui::Separator();
                ImGui::TextUnformatted("The Basel problem");
                TeXRender::imgui_formula(canvas, basel);
                ImGui::Separator();
                ImGui::Text("Glyph atlas: %dx%d, %zu glyphs", canvas.atlas_size(), canvas.atlas_size(), canvas.cached_glyphs());
            }

            ImGui::End();
            ImGui::Render();
            SDL_SetRenderDrawColor(renderer, 246, 247, 251, 255);
            SDL_RenderClear(renderer);
            ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
            SDL_RenderPresent(renderer);
        }
    }

    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    // The backend has released the retired glyph atlas, so free it before the context goes away.
    TeXRender::imgui_collect_retired_textures();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

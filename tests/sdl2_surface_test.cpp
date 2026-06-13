#include <cstdio>
#include <memory>
#include <vector>
#include <SDL.h>
#include <texgui/latex_document.hpp>
#include <texgui/sdl2_surface.hpp>

#ifndef TEXGUI_RES_DIR
#define TEXGUI_RES_DIR "res"
#endif

namespace
{
    struct SdlCleanup
    {
        ~SdlCleanup()
        {
            TexGui::set_default_sdl_renderer(nullptr);
            SDL_Quit();
        }
    };

    bool is_formula_pixel(const unsigned char r, const unsigned char g, const unsigned char b, const unsigned char a)
    {
        return a != 0 && (r < 230 || g < 230 || b < 230);
    }
} // namespace

int main()
{
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        std::fprintf(stderr, "SDL_Init failed, skipping SDL2 render smoke: %s\n", SDL_GetError());
        return 0;
    }
    const auto cleanup = SdlCleanup{};

    auto* const window =
        SDL_CreateWindow("texgui SDL2 test", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 320, 180, SDL_WINDOW_HIDDEN);
    if (!window)
    {
        std::fprintf(stderr, "SDL_CreateWindow failed, skipping SDL2 render smoke: %s\n", SDL_GetError());
        return 0;
    }

    auto* const renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer)
    {
        std::fprintf(stderr, "SDL_CreateRenderer failed, skipping SDL2 render smoke: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        return 0;
    }

    auto exit_code = 0;
    {
        TexGui::set_default_sdl_renderer(renderer);
        const auto document = TexGui::LatexDocument{TEXGUI_RES_DIR};
        const auto formula = document.render(R"(\sqrt{x+1})", 44.f, 0xff101820);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);
        formula.draw(24, 24);

        auto pixels = std::vector<unsigned char>(320u * 180u * 4u);
        if (SDL_RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_RGBA32, pixels.data(), 320 * 4) != 0)
        {
            std::fprintf(stderr, "SDL_RenderReadPixels failed: %s\n", SDL_GetError());
            exit_code = 1;
        }

        auto formula_pixels = 0;
        if (exit_code == 0)
        {
            for (auto i = std::size_t{0}; i + 3 < pixels.size(); i += 4)
            {
                if (is_formula_pixel(pixels[i + 0], pixels[i + 1], pixels[i + 2], pixels[i + 3]))
                    ++formula_pixels;
            }

            if (formula_pixels == 0)
            {
                std::fprintf(stderr, "SDL2 render produced no non-background pixels\n");
                exit_code = 1;
            }
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    return exit_code;
}

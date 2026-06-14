#include <cstdio>
#include <SDL.h>
#include <texrender/latex_document.hpp>
#include <texrender/sdl2_surface.hpp>

#ifndef TEXRENDER_RES_DIR
#define TEXRENDER_RES_DIR "res"
#endif

int main()
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    auto* const window =
        SDL_CreateWindow("texrender SDL2", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 760, 420, SDL_WINDOW_SHOWN);
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

    TeXRender::set_default_sdl_renderer(renderer);

    {
        const auto document = TeXRender::LatexDocument{TEXRENDER_RES_DIR};
        const auto formula = document.render(R"(\sum_{n=1}^{\infty}\frac{1}{n^2}=\frac{\pi^2}{6})", 46.f, 0xff101820);

        auto running = true;
        while (running)
        {
            auto event = SDL_Event{};
            while (SDL_PollEvent(&event) != 0)
            {
                if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE))
                    running = false;
            }

            int width = 0;
            int height = 0;
            SDL_GetRendererOutputSize(renderer, &width, &height);
            SDL_SetRenderDrawColor(renderer, 246, 247, 251, 255);
            SDL_RenderClear(renderer);

            const auto x = (width - formula.width()) / 2;
            const auto y = (height - formula.height()) / 2;
            formula.draw(x, y);

            SDL_RenderPresent(renderer);
        }
    }

    TeXRender::set_default_sdl_renderer(nullptr);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

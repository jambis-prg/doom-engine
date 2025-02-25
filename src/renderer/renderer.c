#include "renderer.h"
#include "window.h"

static SDL_Renderer *renderer = NULL;

bool r_init()
{
    SDL_Window *win = (SDL_Window*)w_get_handler();

    renderer = SDL_CreateRenderer(win, 0, SDL_RENDERER_ACCELERATED);

    if (renderer == NULL)
        return false;

    return true;
}

void r_begin_draw()
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
}

void r_end_draw()
{
    SDL_RenderPresent(renderer);
}

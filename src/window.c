#include "window.h"

static SDL_Window *window = NULL;

bool w_init(uint16_t scrn_w, uint16_t scrn_h)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
        return false;

    window = SDL_CreateWindow(
        "Doom", 
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
        scrn_w, scrn_h, 
        SDL_WINDOW_SHOW
    );

    if (window == NULL)
        return false;

    return true;
}

bool w_handle_events()
{
    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_QUIT:
            return false;
        default:
            break;
        }
    }

    return true;
}

void w_shutdown()
{
}

void *w_get_handler()
{
    return window;
}

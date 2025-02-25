#include "game_core.h"
#include "window.h"
#include "renderer/renderer.h"

typedef struct _game_core
{
    bool is_running, is_paused;
} game_core_t;

static game_core_t game_manager = {0};

bool g_init(uint16_t scrn_w, uint16_t scrn_h)
{
    game_manager.is_running = true;
    game_manager.is_paused = false;

    if (!w_init(scrn_w, scrn_h))
        return false;

    if (!r_init())
        return false;

    return true;
}

void g_run()
{
    while (game_manager.is_running && w_handle_events()) 
    {
        r_begin_draw();


        r_end_draw();
    }
}

void g_shutdown()
{
    r_shutdown();
    w_shutdown();
}

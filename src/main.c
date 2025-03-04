#include "core/game_core.h"

int main()
{
    if (g_init(1280, 960))
    {
        g_run();

        g_shutdown();
    }
    return 0;
}
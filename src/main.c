#include "core/game_core.h"

int main()
{
    if (g_init(1024, 1024))
    {
        g_run();

        g_shutdown();
    }
    return 0;
}
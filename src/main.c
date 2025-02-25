#include "game_core.h"

int main()
{
    if (g_init(1920, 1080))
    {
        g_run();

        g_shutdown();
    }
    return 0;
}
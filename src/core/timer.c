#include "timer.h"
#include <SDL2/SDL.h>

typedef struct _time_manager
{
    double time, delta_time, animation_accum;
    uint64_t last, now, ticks, animation_ticks;
} time_manager_t;

static time_manager_t time_manager = {0};

void t_start()
{
    time_manager.last = SDL_GetPerformanceCounter();
    time_manager.now = 0;
    time_manager.ticks = 0;
    time_manager.animation_ticks = 0;
}

void t_update()
{
    time_manager.now = SDL_GetPerformanceCounter();
    time_manager.delta_time = (double)((time_manager.now - time_manager.last) * 1000 / (double)SDL_GetPerformanceFrequency()) * 0.001;
    time_manager.last = time_manager.now;
    time_manager.time += time_manager.delta_time;
    time_manager.ticks++;

    time_manager.animation_accum += time_manager.delta_time;
    while (time_manager.animation_accum >= ANIMATION_TICK)
    {
        time_manager.animation_accum -= ANIMATION_TICK;
        time_manager.animation_ticks++;
    }
}

double t_get_time()
{
    return time_manager.time;
}

double t_get_delta_time()
{
    return time_manager.delta_time;
}

uint64_t t_get_tick()
{
    return time_manager.ticks;
}

uint64_t t_get_animation_tick()
{
    return time_manager.animation_ticks;
}

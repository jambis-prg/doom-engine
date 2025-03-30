#ifndef TIMER_H_INCLUDED
#define TIMER_H_INCLUDED

#include "typedefs.h"

#define ANIMATION_TICK (4 / 35.f)

void t_start();
void t_update();
double t_get_time();
double t_get_delta_time();
uint64_t t_get_tick();
uint64_t t_get_animation_tick();

#endif
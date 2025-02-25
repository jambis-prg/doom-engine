#ifndef GAME_CORE_H_INCLUDED
#define GAME_CORE_H_INCLUDED

#include "typedefs.h"

bool g_init(uint16_t scrn_w, uint16_t scrn_h);
void g_run();
void g_shutdown();

#endif
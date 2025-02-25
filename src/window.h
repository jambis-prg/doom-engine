#ifndef WINDOW_H_INCLUDED
#define WINDOW_H_INCLUDED

#include "typedefs.h"

bool w_init(uint16_t scrn_w, uint16_t scrn_h);
bool w_handle_events();
void *w_get_handler();
void w_shutdown();


#endif
#ifndef DEVICE_H_INCLUDED
#define DEVICE_H_INCLUDED

#include "typedefs.h"
#include "7_seg.h"
#include "lcd.h"

void d_init();

uint8_t d_button_read();

uint32_t d_switch_read();

void d_write_green_leds(int32_t i);
void d_write_red_leds(int32_t i);

void d_shutdown();
	
#endif

#include "placa_init.h"
#include "ioctl_cmds.h"
#include <stdint.h>

int file_d = 0;
void init_bt(int fd){
	file_d = fd;
}

uint8_t button_read(){
	uint8_t r = 0, e = 0;
	ioctl(file_d, RD_PBUTTONS);
	e = read(file_d, &r, sizeof(r));
	return r;
}

uint32_t switch_read(){
	uint32_t r = 0, e = 0;
	ioctl(file_d, RD_SWITCHES);
	e = read(file_d, &r, sizeof(r));
	return r;
}

void write_green_leds(int i){
	int retval;
	ioctl(file_d, WR_GREEN_LEDS);
	retval = write(file_d, &i, sizeof(i));
}

int init_all(){
	int fd = open("/dev/mydev", O_RDWR);
	seg7_init(fd);
	lcd_init (fd);
	init_bt(fd);
	int i = 0;
	ioctl(file_d, WR_GREEN_LEDS);
	i = write(file_d, &i, sizeof(i));
	i = 0;
	ioctl(file_d, WR_RED_LEDS);
	i = write(file_d, &i, sizeof(i));
	
	return fd;
}

#include "device.h"
#include "ioctl_cmds.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>	/* ioctl() */
static int32_t file_id = 0;

void d_init()
{
	file_id = open("/dev/mydev", O_RDWR);

	seg7_init(file_id);
	lcd_init (file_id);
	
	int i = 0;
	ioctl(file_id, WR_GREEN_LEDS);
	write(file_id, &i, sizeof(i));
	
	ioctl(file_id, WR_RED_LEDS);
	write(file_id, &i, sizeof(i));
}

uint8_t d_button_read()
{
	uint8_t r = 0, e = 0;
	ioctl(file_id, RD_PBUTTONS);
	e = read(file_id, &r, sizeof(r));
	return r;
}

uint32_t d_switch_read()
{
	uint32_t r = 0, e = 0;
	ioctl(file_id, RD_SWITCHES);
	e = read(file_id, &r, sizeof(r));
	return r;
}

void d_write_green_leds(int32_t i)
{
	int retval;
	ioctl(file_id, WR_GREEN_LEDS);
	retval = write(file_id, &i, sizeof(i));
}

void d_write_red_leds(int32_t i)
{
	int retval;
	ioctl(file_id, WR_RED_LEDS);
	retval = write(file_id, &i, sizeof(i));
}

void d_shutdown()
{
	close(file_id);
}

#include "lcd.h"
#include "ioctl_cmds.h"
#include <stdio.h>
#include <unistd.h>	/* close() read() write() */
#include <fcntl.h>	/* open() */
#include <sys/ioctl.h>	/* ioctl() */

static int file_id = 0;

/* Quick helper function for single byte transfers */
static void write_byte(uint16_t val) 
{
    val = val | 0x800;
    ioctl(file_id, WR_LCD_DISPLAY);
    write(file_id, &val, sizeof(val));
}

static void lcd_toggle_enable(uint8_t val, int mode) 
{
    // Toggle enable pin on LCD display
    // We cannot do this too quickly or things don't work
    write_byte(val | (mode << 10) | 0x100);
    usleep(LCD_DELAY_US);
    write_byte(val | (mode << 10));
    usleep(LCD_DELAY_US);
}


void lcd_init(int fd) 
{
	file_id = fd;
	lcd_toggle_enable(0x08, LCD_COMMAND);
    lcd_toggle_enable(0x0f, LCD_COMMAND);
    lcd_toggle_enable(LCD_ENTRYMODESET | LCD_ENTRYLEFT, LCD_COMMAND);
    lcd_toggle_enable(LCD_FUNCTIONSET | 0x10 | LCD_2LINE, LCD_COMMAND);
    lcd_clear();
    lcd_set_cursor(0, 0);
}

void lcd_clear(void) 
{
    lcd_toggle_enable(LCD_CLEARDISPLAY, LCD_COMMAND);
}

// go to location on LCD
void lcd_set_cursor(int line, int position) 
{
    int val = (line == 0) ? 0x80 + position : 0xC0 + position;
    lcd_toggle_enable(val, LCD_COMMAND);
}

void lcd_char(char val) 
{
    lcd_toggle_enable(val, LCD_CHARACTER);
}

void lcd_string(const char *s) 
{
    while (*s)
        lcd_char(*s++);
}

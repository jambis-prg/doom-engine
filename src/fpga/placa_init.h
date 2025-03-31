#ifndef INIT
#define INIT
	void init_bt(int fd);
	
	uint8_t button_read();
	
	uint32_t switch_read();
	
	void write_green_leds(int i);
	
	int init_all();
	
#endif

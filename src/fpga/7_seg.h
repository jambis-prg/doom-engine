#ifndef SEVEN_SEG
#define SEVEN_SEG


void seg7_init(int fd);
void seg7_reset(int idx);
void seg7_write_single(int seg, int number, int _reset);
int seg7_convert_digit(int n);

void seg7_write_str(char *number);
void seg7_write(int number);

void seg7_switch_base(int b);

#endif

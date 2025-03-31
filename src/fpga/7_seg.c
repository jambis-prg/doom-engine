#include "7_seg.h"
#include "ioctl_cmds.h"
#include <stdint.h>
#include <fcntl.h> /* open() */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h> /* ioctl() */
#include <sys/types.h>
#include <unistd.h> /* close() read() write() */

static int file_d = 0;
int32_t current_R = 0;
int32_t current_L = 0;

void seg7_init(int fd) {
  file_d = fd;
  seg7_reset(0);
  seg7_reset(1);
}

void seg7_reset(int idx) {
  int reset = 0xfffffff;
  // 0 vai ser o da direita da frente da placa e 1 o da esquerda
  if (idx) {
    ioctl(file_d, WR_R_DISPLAY);
  } else {
    ioctl(file_d, WR_L_DISPLAY);
  }
  write(file_d, &reset, sizeof(reset));
}
void seg7_write_single(int seg, int number, int _reset) {
  // o seg e o index do seg comecando da direita de frente para a placa
  // em 0-7, o reset ele reseta os outros ou nao
  // R e L ta ao contrario, de costas para a placa, frente para as saidas de
  // cabos se reset 1 ele limpa se nao ele mantem os outros
  if (_reset) {
    current_L = 0;
    current_R = 0;
    seg7_reset(1);
    seg7_reset(0);
  }
  if (seg > 7) {
    return;
  }

  if (seg > 3) {
  } else {
  }

  int d = seg7_convert_digit(number) << (7 * (seg % 4));
  int mask =
      ~(0x7f << (7 * (seg % 4))); // para manter ou nao apenas o digto escolhido

  // TODO: Testar funcao de buffer_write, para ele escrever os current_*
  if (seg > 3) {
    d = (current_L & mask) | d;
    ioctl(file_d, WR_R_DISPLAY);
    current_L = d;

  } else {
    d = (current_R & mask) | d;
    ioctl(file_d, WR_L_DISPLAY);
    current_R = d;
  }

  d = ~d;
  write(file_d, &d, sizeof(d));
}

int seg7_convert_digit(int n) {
  int ret = 0;
  switch (n) {  // segmentos estao na ordem 0gfedcba, sendo o 0 o ponto nao mapeado
  case 1:
    ret = 0b00000110;
    break;
  case 2:
    ret = 0b01011011;
    break;
  case 3:
    ret = 0b01001111;
    break;
  case 4:
    ret = 0b01100110;
    break;
  case 5:
    ret = 0b01101101;
    break;
  case 6:
    ret = 0b01111101;
    break;
  case 7:
    ret = 0b00000111;
    break;
  case 8:
    ret = 0b01111111;
    break;
  case 9:
    ret = 0b01100111;
    break;
  case 10: // A
    ret = 0b01110111;
    break;
  case 11: // B
    ret = 0b01111100;
    break;
  case 12: // C
    ret = 0b00111001;
    break;
  case 13: // D
    ret = 0b01011110;
    break;
  case 14: // E
    ret = 0b01111001;
    break;
  case 15: // F
    ret = 0b01110001;
    break;
  default:
    ret = 0b00111111;
    break;
  }
  return ret;
}

void seg7_write_str(char *number) {
  // identifica a base certa
  int n = strtol(number, NULL, 0);
  seg7_write(n);
}

int BASE_S = 10;

void seg7_write(int number) {
  int digi_qtd = 0;
  u_int64_t tmp = 0;
  while (digi_qtd <= 7 && number != 0) {
    tmp |= (u_int64_t)seg7_convert_digit(number % BASE_S) << (7 * (digi_qtd++));
    number = number / BASE_S;
  }
  // usar mascara para pegar os 28 a direita e 28 a esquerda
  current_R = (int32_t)(tmp & 0xFFFFFFF);
  current_L = (int32_t)((tmp >> 28) & 0xFFFFFFF);
  
  int32_t b = ~current_R;
  ioctl(file_d, WR_L_DISPLAY);
  write(file_d, &b, sizeof(b));
  b = ~current_L;
  ioctl(file_d, WR_R_DISPLAY);
  write(file_d, &b, sizeof(b));
}

void seg7_switch_base(int b) {
  if (b != 0) {
    BASE_S = b;
  }
}

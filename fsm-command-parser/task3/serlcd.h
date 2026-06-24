/*
 * serlcd.h  —  SerLCD USART2 driver header (Task 3)
 *
 * Authors: Faid Faisal
 * Created: April 28, 2026
 */

#ifndef SERLCD_H
#define SERLCD_H

#include <stdint.h>

#define BUFFER_SIZE    82
#define CHARS_PER_LINE 20

extern char dsp_buff1[], dsp_buff2[], dsp_buff3[], dsp_buff4[];
extern char set_buff1[], set_buff2[], set_buff3[], set_buff4[];

void USART_init(void);
void USART_send_string(void);
void USART_send_settings(void);

#endif /* SERLCD_H */

/*
 * fsm.h  —  Extended FSM interface (Task 3)
 *
 * Authors: Faid Faisal
 * Created: April 28, 2026
 *
 * Commands supported:
 *   Cn=hh<CR>  – write hex value hh to control register n (1-9)
 *   P<CR>      – toggle display page between measurements and settings
 */

#ifndef FSM_H
#define FSM_H

#include <stdint.h>

#define CNTL_BASE_ADDR 0x0F

extern volatile uint8_t cntl_regs[10];
extern volatile uint8_t pg_num;

void fsm_init(void);
void fsm_parse(uint8_t c);
void toggle_page(void);

#endif /* FSM_H */

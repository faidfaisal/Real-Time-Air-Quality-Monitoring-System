/*
 * fsm.h  —  Table-driven FSM interface (Task 2)
 *
 * Authors: Faid Faisal & Melchai Mathew
 * Created: April 28, 2026
 *
 * Parses terminal commands of the form  Cn=hh<CR>
 * where n = register number (1-9) and hh = two hex digits.
 */

#ifndef FSM_H
#define FSM_H

#include <stdint.h>

/* CNTL1 lives at 0x0F; CNTL_n is at (CNTL_BASE_ADDR + n - 1) */
#define CNTL_BASE_ADDR 0x0F

extern volatile uint8_t cntl_regs[10]; // cntl_regs[1..9] mirror CNTL1..CNTL9

void fsm_init(void);         // Initialise FSM state and shadow registers
void fsm_parse(uint8_t c);   // Feed one character to the FSM

#endif /* FSM_H */

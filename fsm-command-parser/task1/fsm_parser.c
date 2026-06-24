/*
 * fsm_parser.c  —  Lab 12 Task 1: Switch-Case FSM Command Parser
 *
 * Created: April 28, 2026
 * Authors: Faid Faisal
 *
 * Description:
 * Receives characters from a terminal via USART3 interrupts and uses a
 * switch-case finite state machine to parse commands of the form Cn=hh<CR>,
 * where n is a register number (1-9) and hh is a two-digit hex value.
 * When a complete command is received, execute_command() is called.
 *
 * FSM States:
 *   IDLE  → saw 'C' → STATE_C
 *   STATE_C   → saw '1'-'9' → STATE_REG   (stores reg number)
 *   STATE_REG → saw '='     → STATE_EQUAL
 *   STATE_EQUAL → saw hex   → STATE_VAL1  (stores high nibble)
 *   STATE_VAL1  → saw hex   → STATE_VAL2  (ORs in low nibble)
 *   STATE_VAL2  → saw '\r'  → executes command, back to IDLE
 */

#define F_CPU 4000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <util/delay.h>

typedef enum {
    STATE_IDLE,
    STATE_C,
    STATE_REG,
    STATE_EQUAL,
    STATE_VAL1,
    STATE_VAL2
} state_t;

volatile state_t current_state = STATE_IDLE;
volatile uint8_t reg_num = 0;
volatile uint8_t value   = 0;

void    USART3_init(void);
void    fsm_parse(uint8_t c);
uint8_t hex_to_val(uint8_t c);

void execute_command(uint8_t reg, uint8_t val)
{
    /* Set breakpoint here during debugging to verify parsed values. */
    (void)reg; (void)val;
}

void USART3_init(void)
{
    PORTB.DIR &= ~PIN3_bm;           // PB3 = RX (input)
    PORTB.DIR |=  PIN2_bm;           // PB2 = TX (output)
    USART3.BAUD  = 1667;             // 9600 baud @ 4 MHz
    USART3.CTRLC = USART_CHSIZE_8BIT_gc;
    USART3.CTRLB = USART_RXEN_bm;
    USART3.CTRLA = USART_RXCIE_bm;
}

ISR(USART3_RXC_vect)
{
    uint8_t data = USART3.RXDATAL;
    fsm_parse(data);
}

uint8_t hex_to_val(uint8_t c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return 0;
}

void fsm_parse(uint8_t c)
{
    switch (current_state) {
    case STATE_IDLE:
        if (c == 'C' || c == 'c')
            current_state = STATE_C;
        break;

    case STATE_C:
        if (c >= '1' && c <= '9') {
            reg_num = c - '0';
            current_state = STATE_REG;
        } else {
            current_state = STATE_IDLE;
        }
        break;

    case STATE_REG:
        if (c == '=')
            current_state = STATE_EQUAL;
        else
            current_state = STATE_IDLE;
        break;

    case STATE_EQUAL:
        if ((c >= '0' && c <= '9') ||
            (c >= 'A' && c <= 'F') ||
            (c >= 'a' && c <= 'f')) {
            value = hex_to_val(c) << 4;
            current_state = STATE_VAL1;
        } else {
            current_state = STATE_IDLE;
        }
        break;

    case STATE_VAL1:
        if ((c >= '0' && c <= '9') ||
            (c >= 'A' && c <= 'F') ||
            (c >= 'a' && c <= 'f')) {
            value |= hex_to_val(c);
            current_state = STATE_VAL2;
        } else {
            current_state = STATE_IDLE;
        }
        break;

    case STATE_VAL2:
        if (c == '\r')
            execute_command(reg_num, value);
        current_state = STATE_IDLE;
        break;

    default:
        current_state = STATE_IDLE;
        break;
    }
}

int main(void)
{
    USART3_init();
    sei();
    while (1) { }   // All work done in ISR
}

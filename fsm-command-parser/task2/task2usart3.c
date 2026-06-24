/*
 * task2usart3.c  —  USART3 Interrupt-Driven Receiver (Task 2)
 *
 * Created: April 28, 2026
 * Authors: Faid Faisal
 *
 * Configures USART3 at 9600 8N1 on PB2 (TX) / PB3 (RX).
 * Each received character is passed directly to fsm_parse().
 */

#define F_CPU 4000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include "fsm.h"

void USART3_init(void)
{
    PORTB.DIR &= ~PIN3_bm;           // PB3 = RX, input
    PORTB.DIR |=  PIN2_bm;           // PB2 = TX, output
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

/*
 * task3usart3.c  —  USART3 Interrupt-Driven Receiver (Task 3)
 *
 * Created: April 28, 2026
 * Authors: Faid Faisal & Melchai Mathew
 */

#define F_CPU 4000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include "fsm.h"

void USART3_init(void)
{
    PORTB.DIR &= ~PIN3_bm;
    PORTB.DIR |=  PIN2_bm;
    USART3.BAUD  = 1667;
    USART3.CTRLC = USART_CHSIZE_8BIT_gc;
    USART3.CTRLB = USART_RXEN_bm;
    USART3.CTRLA = USART_RXCIE_bm;
}

ISR(USART3_RXC_vect)
{
    uint8_t data = USART3.RXDATAL;
    fsm_parse(data);
}

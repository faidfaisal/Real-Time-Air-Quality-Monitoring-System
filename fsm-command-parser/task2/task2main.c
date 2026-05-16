/*
 * task2main.c  —  Lab 12 Task 2 Main Loop
 *
 * Created: April 28, 2026
 * Authors: Faid Faisal & Melchai Mathew
 *
 * Description:
 * Main control loop: continuously acquires AK9723AJ sensor readings and
 * pushes them to the SerLCD display while USART3 interrupts feed incoming
 * terminal characters to the table-driven FSM, which writes updated values
 * to sensor control registers in real time.
 */

#define F_CPU 4000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "ak9723.h"
#include "fsm.h"

/* Forward declaration for USART2/SerLCD functions defined in serlcd.c */
void USART_init(void);
void USART_send_string(void);

/* Forward declaration for USART3 init defined in task2usart3.c */
void USART3_init(void);

int main(void)
{
    PORTA.DIRCLR = PIN4_bm;   // PA4 = INTN (data-ready, active low)
    PORTA.DIRSET = PIN7_bm;   // PA7 = overcurrent LED

    TWI0_AK9723_init();
    USART_init();
    USART3_init();
    fsm_init();
    sei();

    while (1)
    {
        AK9723_TWI0_init();
        TWI0_AK9723_byte_write(AK9723AJ_ADDR, 0x14, 0x02);
        while (PORTA.IN & PIN4_bm);
        TWI0_AK9723_block_read(AK9723AJ_ADDR, 0x04);
        error_check();
        convert_store_vals();
        USART_send_string();
        _delay_ms(1500);
    }
}

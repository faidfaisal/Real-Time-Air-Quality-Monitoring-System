/*
 * task3main.c  —  Lab 12 Task 3 Main Loop
 *
 * Created: April 28, 2026
 * Authors: Faid Faisal & Melchai Mathew
 *
 * Description:
 * Main loop for the full AK9723AJ tuning system. Continuously reads sensor
 * data and updates the SerLCD. The display page shown depends on pg_num,
 * which is toggled by the FSM each time a P<CR> command is received via
 * the USART3 interrupt.
 *
 *   pg_num == 0  →  Page 0: IR1, IR2, Vf measurements
 *   pg_num == 1  →  Page 1: C1–C9 control register values
 */

#define F_CPU 4000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "ak9723.h"
#include "serlcd.h"
#include "fsm.h"

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

        if (pg_num == 0)
            USART_send_string();    // Measurements page
        else
            USART_send_settings();  // Control register settings page

        _delay_ms(1500);
    }
}

/*
 * task3serlcd.c  —  USART2 SerLCD Driver with Dual-Page Support (Task 3)
 *
 * Created: April 28, 2026
 * Authors: Faid Faisal 
 *
 * Description:
 * Interrupt-driven USART2 transmit driver for the SparkFun 4×20 SerLCD.
 * Supports two display pages loaded via a ring buffer:
 *   Page 0 – IR1, IR2, Vf measurements
 *   Page 1 – C1..C9 control register values (built from cntl_regs[] shadow)
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include "serlcd.h"
#include "fsm.h"

/* ── Display buffers ────────────────────────────────────────────────────── */
char dsp_buff1[CHARS_PER_LINE + 1];
char dsp_buff2[CHARS_PER_LINE + 1];
char dsp_buff3[CHARS_PER_LINE + 1];
char dsp_buff4[CHARS_PER_LINE + 1];

char set_buff1[CHARS_PER_LINE + 1];
char set_buff2[CHARS_PER_LINE + 1];
char set_buff3[CHARS_PER_LINE + 1];
char set_buff4[CHARS_PER_LINE + 1];

/* ── TX ring buffer ─────────────────────────────────────────────────────── */
static char     tx_buffer[BUFFER_SIZE];
static volatile uint8_t tx_head = 0;
static volatile uint8_t tx_tail = 0;

/* ── USART2 init ────────────────────────────────────────────────────────── */
void USART_init(void)
{
    PORTF.DIRSET = PIN4_bm;
    PORTMUX.USARTROUTEA = PORTMUX_USART2_ALT1_gc;
    USART2.BAUD  = 1667;
    USART2.CTRLC = USART_CHSIZE_8BIT_gc;
    USART2.CTRLB = USART_TXEN_bm;
}

/* ── Internal helpers ───────────────────────────────────────────────────── */
static void enqueue(char c)
{
    tx_buffer[tx_head++] = c;
}

static void load_buffers(char *b1, char *b2, char *b3, char *b4)
{
    cli();
    tx_head = tx_tail = 0;
    enqueue('|'); enqueue('-');   // SerLCD: clear + home
    for (uint8_t i = 0; b1[i]; i++) enqueue(b1[i]);
    for (uint8_t i = 0; b2[i]; i++) enqueue(b2[i]);
    for (uint8_t i = 0; b3[i]; i++) enqueue(b3[i]);
    for (uint8_t i = 0; b4[i]; i++) enqueue(b4[i]);
    sei();
    USART2.CTRLA |= USART_DREIE_bm;
}

static void build_settings_page(void)
{
    sprintf(set_buff1, "C1=%02X C2=%02X C3=%02X ", cntl_regs[1], cntl_regs[2], cntl_regs[3]);
    sprintf(set_buff2, "C4=%02X C5=%02X C6=%02X ", cntl_regs[4], cntl_regs[5], cntl_regs[6]);
    sprintf(set_buff3, "C7=%02X C8=%02X C9=%02X ", cntl_regs[7], cntl_regs[8], cntl_regs[9]);
    sprintf(set_buff4, "                    ");
}

/* ── Public API ─────────────────────────────────────────────────────────── */
void USART_send_string(void)
{
    load_buffers(dsp_buff1, dsp_buff2, dsp_buff3, dsp_buff4);
}

void USART_send_settings(void)
{
    build_settings_page();
    load_buffers(set_buff1, set_buff2, set_buff3, set_buff4);
}

/* ── ISR ────────────────────────────────────────────────────────────────── */
ISR(USART2_DRE_vect)
{
    if (tx_head != tx_tail)
        USART2.TXDATAL = tx_buffer[tx_tail++];
    else
        USART2.CTRLA &= ~USART_DREIE_bm;
}

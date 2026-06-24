/*
 * main.c  —  AK9723AJ Measurement & Display System
 *
 * Created: 4/1/2026
 * Authors: Faid Faisal
 *
 * Description:
 * Reads raw 24-bit IR1, IR2, and 16-bit VF ADC values from the AK9723AJ
 * CO₂ sensor over I²C (TWI), converts them to millivolts, and streams the
 * formatted results to a SparkFun 4×20 SerLCD display via USART2 using an
 * interrupt-driven ring buffer.  An overcurrent LED indicator (PA7) is also
 * driven from the ST1 status register.
 *
 * Hardware:
 *   MCU  : AVR128DB48 Curiosity Nano
 *   I²C  : TWI0  — PA2 (SDA), PA3 (SCL)
 *   UART : USART2 ALT1 — PF4 (TX) → SerLCD
 *   INTN : PA4  (active-low data-ready from sensor)
 *   LED  : PA7  (active-low overcurrent indicator)
 */

#define F_CPU 4000000UL
#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

/* ── Constants ──────────────────────────────────────────────────────────── */
#define CHARS_PER_LINE    20
#define BUFFER_SIZE       80
#define OVCUR_DET         0x04
#define MAX_BAUD_AK9723AJ 2
#define USART_BAUD_RATE   1667
#define AK9723AJ_ADDR     0x65

/* ── Prototypes ─────────────────────────────────────────────────────────── */
void    TWI0_AK9723_init(void);
void    AK9723_TWI0_init(void);
void    TWI0_send_start(uint8_t saddr, uint8_t rw);
void    TWI0_write_byte(uint8_t data);
uint8_t TWI0_read_byte(uint8_t ack);
void    TWI0_send_stop(void);
void    TWI0_AK9723_byte_write(uint8_t saddr, uint8_t raddr, uint8_t data);
void    TWI0_AK9723_block_read(uint8_t saddr, uint8_t raddr);
void    USART_init(void);
void    USART_send_string(void);
void    error_check(void);
void    convert_store_vals(void);

/* ── Globals ────────────────────────────────────────────────────────────── */
char tx_buffer[BUFFER_SIZE];
volatile uint8_t tx_head;
volatile uint8_t tx_tail;

volatile struct data_read {
    uint8_t ST1;
    uint8_t IR1L, IR1M, IR1H;
    uint8_t IR2L, IR2M, IR2H;
    uint8_t TMPL, TMPH;
    uint8_t VFL,  VFH;
} meas_readings;

char dsp_buff1[CHARS_PER_LINE + 1];
char dsp_buff2[CHARS_PER_LINE + 1];
char dsp_buff3[CHARS_PER_LINE + 1];
char dsp_buff4[CHARS_PER_LINE + 1];

/* ── Main ───────────────────────────────────────────────────────────────── */
int main(void)
{
    PORTA.DIRCLR = PIN4_bm;   // PA4 = INTN input
    PORTA.DIRSET = PIN7_bm;   // PA7 = LED output
    PORTA.OUTCLR = PIN7_bm;   // LED ON during self-test
    _delay_ms(1000);
    PORTA.OUTSET = PIN7_bm;   // LED OFF

    TWI0_AK9723_init();
    USART_init();
    sei();
    _delay_us(200);

    while (1)
    {
        AK9723_TWI0_init();
        TWI0_AK9723_byte_write(AK9723AJ_ADDR, 0x14, 0x02);
        while (PORTA.IN & PIN4_bm);
        TWI0_AK9723_block_read(AK9723AJ_ADDR, 0x04);
        error_check();
        convert_store_vals();
        USART_send_string();
        _delay_ms(1.5);
    }
}

/* ── TWI ────────────────────────────────────────────────────────────────── */
void TWI0_AK9723_init(void)
{
    TWI0.MBAUD   = MAX_BAUD_AK9723AJ;
    TWI0.MCTRLA  = TWI_ENABLE_bm;
    TWI0.DBGCTRL = TWI_DBGRUN_bm;
    TWI0.MSTATUS = TWI_BUSSTATE_IDLE_gc;
}

void AK9723_TWI0_init(void)
{
    TWI0_AK9723_byte_write(AK9723AJ_ADDR, 0x18, 0xFF);
    TWI0_AK9723_byte_write(AK9723AJ_ADDR, 0x0F, 0xF0);
    TWI0_AK9723_byte_write(AK9723AJ_ADDR, 0x10, 0x00);
    TWI0_AK9723_byte_write(AK9723AJ_ADDR, 0x11, 0x00);
    TWI0_AK9723_byte_write(AK9723AJ_ADDR, 0x16, 0xE0);
    TWI0_AK9723_byte_write(AK9723AJ_ADDR, 0x17, 0x00);
}

void TWI0_send_start(uint8_t saddr, uint8_t rw)
{
    TWI0.MADDR = (saddr << 1) | rw;
    if (rw == 0)
        while (!(TWI0.MSTATUS & TWI_WIF_bm));
    if (TWI0.MSTATUS & TWI_RXACK_bm)
        TWI0.MCTRLB = TWI_MCMD_STOP_gc;
}

void TWI0_write_byte(uint8_t data)
{
    TWI0.MDATA = data;
    while (!(TWI0.MSTATUS & TWI_WIF_bm));
}

uint8_t TWI0_read_byte(uint8_t ack)
{
    TWI0.MCTRLB = TWI_MCMD_RECVTRANS_gc;
    while (!(TWI0.MSTATUS & TWI_RIF_bm));
    uint8_t data = TWI0.MDATA;
    if (ack == 1)
        TWI0.MCTRLB = TWI_MCMD_RECVTRANS_gc;
    else
        TWI0.MCTRLB = TWI_ACKACT_NACK_gc;
    return data;
}

void TWI0_send_stop(void)
{
    TWI0.MCTRLB = TWI_MCMD_STOP_gc;
}

void TWI0_AK9723_byte_write(uint8_t saddr, uint8_t raddr, uint8_t data)
{
    TWI0_send_start(saddr, 0);
    TWI0_write_byte(raddr);
    TWI0_write_byte(data);
    TWI0_send_stop();
}

void TWI0_AK9723_block_read(uint8_t saddr, uint8_t raddr)
{
    volatile uint8_t *arr = (volatile uint8_t *)&meas_readings;
    TWI0_send_start(saddr, 0);
    TWI0_write_byte(raddr);
    TWI0_send_start(saddr, 1);
    for (uint8_t i = 0; i < 10; i++)
        arr[i] = TWI0_read_byte(1);
    arr[10] = TWI0_read_byte(0);
    TWI0_send_stop();
}

/* ── USART ──────────────────────────────────────────────────────────────── */
void USART_init(void)
{
    PORTF.DIRSET = PIN4_bm;
    USART2.BAUD  = USART_BAUD_RATE;
    PORTMUX.USARTROUTEA = PORTMUX_USART2_ALT1_gc;
    USART2.CTRLB = USART_TXEN_bm;
    USART2.CTRLC = USART_CHSIZE_8BIT_gc;
}

void USART_send_string(void)
{
    cli();
    tx_head = tx_tail = 0;
    for (uint8_t i = 0; dsp_buff1[i]; i++) tx_buffer[tx_head++] = dsp_buff1[i];
    for (uint8_t i = 0; dsp_buff2[i]; i++) tx_buffer[tx_head++] = dsp_buff2[i];
    for (uint8_t i = 0; dsp_buff3[i]; i++) tx_buffer[tx_head++] = dsp_buff3[i];
    for (uint8_t i = 0; dsp_buff4[i]; i++) tx_buffer[tx_head++] = dsp_buff4[i];
    sei();
    USART2.CTRLA |= USART_DREIE_bm;
}

/* ── Data Processing ────────────────────────────────────────────────────── */
void error_check(void)
{
    if (meas_readings.ST1 & OVCUR_DET)
        PORTA.OUTCLR = PIN7_bm;   // Overcurrent → LED ON
    else
        PORTA.OUTSET = PIN7_bm;   // Normal → LED OFF
}

void convert_store_vals(void)
{
    /* IR1 */
    int32_t IR1_raw = ((int32_t)meas_readings.IR1H << 16) |
                      ((int32_t)meas_readings.IR1M <<  8) |
                       meas_readings.IR1L;
    if (IR1_raw & 0x800000) IR1_raw |= 0xFF000000;
    char    sign_IR1      = (IR1_raw < 0) ? '-' : '+';
    int32_t mV_IR1        = (int32_t)(((int64_t)IR1_raw * 7500LL) / 8388607LL);
    int32_t whole_IR1     = abs(mV_IR1) / 10;
    int32_t frac_IR1      = abs(mV_IR1) % 10;

    /* IR2 */
    int32_t IR2_raw = ((int32_t)meas_readings.IR2H << 16) |
                      ((int32_t)meas_readings.IR2M <<  8) |
                       meas_readings.IR2L;
    if (IR2_raw & 0x800000) IR2_raw |= 0xFF000000;
    char    sign_IR2      = (IR2_raw < 0) ? '-' : '+';
    int32_t mV_IR2        = (int32_t)(((int64_t)IR2_raw * 7500LL) / 8388607LL);
    int32_t whole_IR2     = abs(mV_IR2) / 10;
    int32_t frac_IR2      = abs(mV_IR2) % 10;

    /* VF */
    int16_t Vf_raw   = (int16_t)(((uint16_t)meas_readings.VFH << 8) |
                                   meas_readings.VFL);
    int32_t mV_Vf    = (int32_t)(((int64_t)Vf_raw * 15000LL) / 32767LL) + 14000;
    char    sign_Vf  = (mV_Vf < 0) ? '-' : '+';
    int32_t whole_Vf = abs(mV_Vf) / 10;
    int32_t frac_Vf  = abs(mV_Vf) % 10;

    sprintf(dsp_buff1, "IR1 = %c%07ld.%01ld mV ", sign_IR1, whole_IR1, frac_IR1);
    sprintf(dsp_buff2, "IR2 = %c%07ld.%01ld mV ", sign_IR2, whole_IR2, frac_IR2);
    sprintf(dsp_buff3, "VF  = %c%07ld.%01ld mV ", sign_Vf,  whole_Vf,  frac_Vf);
    sprintf(dsp_buff4, "                    ");
}

/* ── ISR ────────────────────────────────────────────────────────────────── */
ISR(USART2_DRE_vect)
{
    if (tx_head != tx_tail)
        USART2.TXDATA = tx_buffer[tx_tail++];
    else
        USART2.CTRLA &= ~USART_DREIE_bm;
}

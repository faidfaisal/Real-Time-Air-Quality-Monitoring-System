/*
 * ak9723aj_meas_all_readings.c
 *
 * Created: 04/08/2026
 * Authors: Faid Faisal
 *
 * Description:
 * Full measurement loop: initialises sensor control registers, triggers a
 * single conversion, polls the INTN status pin (PA4) for data-ready, then
 * burst-reads all 11 measurement bytes into a struct. Repeats indefinitely.
 */

#define F_CPU 4000000UL
#include <avr/io.h>
#include <util/delay.h>

#define MAX_BAUD_AK9723AJ 2
#define AK9723AJ_ADDR     0x65

void    TWI0_AK9723_init(void);
void    AK9723_TWI0_init(void);
void    TWI0_send_start(uint8_t saddr, uint8_t rw);
void    TWI0_write_byte(uint8_t data);
uint8_t TWI0_read_byte(uint8_t ack);
void    TWI0_send_stop(void);
void    TWI0_AK9723_byte_write(uint8_t saddr, uint8_t raddr, uint8_t data);
void    TWI0_AK9723_block_read(uint8_t saddr, uint8_t raddr);

volatile struct data_read {
    uint8_t ST1;
    uint8_t IR1L, IR1M, IR1H;
    uint8_t IR2L, IR2M, IR2H;
    uint8_t TMPL, TMPH;
    uint8_t VFL,  VFH;
} meas_readings;

int main(void)
{
    PORTA.DIRCLR = PIN4_bm;   // PA4 = INTN input
    TWI0_AK9723_init();
    _delay_us(200);

    while (!(PORTA.IN & PIN4_bm)); // Wait for sensor ready

    while (1)
    {
        AK9723_TWI0_init();
        TWI0_AK9723_byte_write(AK9723AJ_ADDR, 0x14, 0x02); // Trigger measurement
        while (PORTA.IN & PIN4_bm);                          // Wait data-ready
        TWI0_AK9723_block_read(AK9723AJ_ADDR, 0x04);
        _delay_ms(2);
    }
}

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

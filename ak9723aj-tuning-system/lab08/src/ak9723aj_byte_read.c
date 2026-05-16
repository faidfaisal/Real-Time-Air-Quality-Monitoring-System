/*
 * ak9723aj_byte_read.c
 *
 * Created: 4/1/2026
 * Authors: Faid Faisal & Melchai Mathew
 *
 * Description:
 * Reads multiple registers from the AK9723AJ sensor in a loop using a
 * helper function that performs single-byte I²C reads (START → write addr
 * → repeated START → read → STOP).
 */

#include <avr/io.h>

#define MAX_BAUD_AK9723AJ 2
#define AK9723AJ_ADDR     0x65

void    TWI0_AK9723_init(void);
void    TWI0_send_start(uint8_t saddr, uint8_t rw);
void    TWI0_write_byte(uint8_t data);
uint8_t TWI0_read_byte(uint8_t ack);
void    TWI0_send_stop(void);
uint8_t TWI0_AK9723_byte_read(uint8_t saddr, uint8_t raddr);

volatile uint8_t reg_value;

int main(void)
{
    TWI0_AK9723_init();

    while (1)
    {
        for (uint8_t i = 0; i < 0x19; i++)
            reg_value = TWI0_AK9723_byte_read(AK9723AJ_ADDR, i);
    }
}

void TWI0_AK9723_init(void)
{
    TWI0.MBAUD   = MAX_BAUD_AK9723AJ;
    TWI0.MCTRLA  = TWI_ENABLE_bm;
    TWI0.DBGCTRL = TWI_DBGRUN_bm;
    TWI0.MSTATUS = TWI_BUSSTATE_IDLE_gc;
}

void TWI0_send_start(uint8_t saddr, uint8_t rw)
{
    TWI0.MADDR = (saddr << 1) | rw;
    if (rw == 0)
        while (!(TWI0.MSTATUS & TWI_WIF_bm));
    else
        while (!(TWI0.MSTATUS & TWI_RIF_bm));
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

uint8_t TWI0_AK9723_byte_read(uint8_t saddr, uint8_t raddr)
{
    TWI0_send_start(saddr, 0);
    TWI0_write_byte(raddr);
    TWI0_send_start(saddr, 1);
    uint8_t data = TWI0_read_byte(0);
    TWI0_send_stop();
    return data;
}

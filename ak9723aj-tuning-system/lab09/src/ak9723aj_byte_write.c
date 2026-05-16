/*
 * ak9723aj_byte_write.c
 *
 * Created: 04/08/2026
 * Authors: Faid Faisal & Melchai Mathew
 *
 * Description:
 * Initializes TWI (I2C) on the AVR128DB48 and performs byte-write operations
 * to the AK9723AJ sensor, writing a fixed value (0x67) to each control
 * register in the range 0x0F–0x18 in a continuous loop.
 */

#include <avr/io.h>

#define MAX_BAUD_AK9723AJ 2
#define AK9723AJ_ADDR     0x65

void    TWI0_AK9723_init(void);
void    TWI0_send_start(uint8_t saddr, uint8_t rw);
void    TWI0_write_byte(uint8_t data);
uint8_t TWI0_read_byte(uint8_t ack);
void    TWI0_send_stop(void);
void    TWI0_AK9723_byte_write(uint8_t saddr, uint8_t raddr, uint8_t data);

int main(void)
{
    TWI0_AK9723_init();

    while (1)
    {
        for (uint8_t i = 0x0F; i < 0x19; i++)
            TWI0_AK9723_byte_write(AK9723AJ_ADDR, i, 0x67);
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

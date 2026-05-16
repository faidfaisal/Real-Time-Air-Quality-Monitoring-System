/*
 * ak9723aj_twi.c
 *
 * Created: 4/1/2026
 * Authors: Faid Faisal & Melchai Mathew
 *
 * Description:
 * Initializes the AVR TWI (I2C) interface and communicates with the AK9723AJ
 * sensor. Demonstrates reading a single register over I²C using START, write,
 * repeated START, and read sequences.
 */

#include <avr/io.h>

#define MAX_BAUD_AK9723AJ 2
#define AK9723AJ_ADDR     0x65

void TWI0_AK9723_init(void);
void TWI0_send_start(uint8_t rw);
void TWI0_write_byte(uint8_t data);
uint8_t TWI0_read_byte(uint8_t ack);
void TWI0_send_stop(void);

int main(void)
{
    TWI0_AK9723_init();

    TWI0_send_start(0);           // START + write mode
    TWI0_write_byte(0x00);        // Register address 0x00
    TWI0_send_start(1);           // Repeated START + read mode
    uint8_t reg_val = TWI0_read_byte(0); // Read one byte, NACK
    TWI0_send_stop();

    while (1) { }
}

void TWI0_AK9723_init(void)
{
    TWI0.MBAUD   = MAX_BAUD_AK9723AJ;
    TWI0.MCTRLA  = TWI_ENABLE_bm;
    TWI0.DBGCTRL = TWI_DBGRUN_bm;
    TWI0.MSTATUS = TWI_BUSSTATE_IDLE_gc;
}

void TWI0_send_start(uint8_t rw)
{
    TWI0.MADDR = (AK9723AJ_ADDR << 1) | rw;
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

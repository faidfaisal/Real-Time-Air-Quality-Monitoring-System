/*
 * task3_ak9723.c  —  AK9723AJ TWI Driver (Task 3)
 * Identical to Task 2 driver; separated per task for clarity.
 *
 * Authors: Faid Faisal & Melchai Mathew
 */

#include <avr/io.h>
#include <stdio.h>
#include "ak9723.h"

volatile data_read_t meas_readings;

extern char dsp_buff1[], dsp_buff2[], dsp_buff3[], dsp_buff4[];

static void TWI0_send_start(uint8_t saddr, uint8_t rw)
{
    TWI0.MADDR = (saddr << 1) | rw;
    if (rw == 0)
        while (!(TWI0.MSTATUS & TWI_WIF_bm));
    if (TWI0.MSTATUS & TWI_RXACK_bm)
        TWI0.MCTRLB = TWI_MCMD_STOP_gc;
}

static void TWI0_write_byte(uint8_t data)
{
    TWI0.MDATA = data;
    while (!(TWI0.MSTATUS & TWI_WIF_bm));
}

static uint8_t TWI0_read_byte(uint8_t ack)
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

static void TWI0_send_stop(void)
{
    TWI0.MCTRLB = TWI_MCMD_STOP_gc;
}

void TWI0_AK9723_init(void)
{
    TWI0.MBAUD   = 2;
    TWI0.MCTRLA  = TWI_ENABLE_bm;
    TWI0.MSTATUS = TWI_BUSSTATE_IDLE_gc;
}

void AK9723_TWI0_init(void)
{
    TWI0_AK9723_byte_write(AK9723AJ_ADDR, 0x18, 0xFF);
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

void error_check(void)
{
    if (meas_readings.ST1 & OVCUR_DET)
        PORTA.OUTCLR = PIN7_bm;
    else
        PORTA.OUTSET = PIN7_bm;
}

void convert_store_vals(void)
{
    int32_t IR1_raw = ((int32_t)meas_readings.IR1H << 16) |
                      ((int32_t)meas_readings.IR1M <<  8) | meas_readings.IR1L;
    if (IR1_raw & 0x800000) IR1_raw |= 0xFF000000;
    char    sign_IR1  = (IR1_raw < 0) ? '-' : '+';
    int32_t mV_IR1    = (int32_t)(((int64_t)IR1_raw * 7500LL) / 8388607LL);
    int32_t whole_IR1 = (mV_IR1 < 0 ? -mV_IR1 : mV_IR1) / 10;
    int32_t frac_IR1  = (mV_IR1 < 0 ? -mV_IR1 : mV_IR1) % 10;

    int32_t IR2_raw = ((int32_t)meas_readings.IR2H << 16) |
                      ((int32_t)meas_readings.IR2M <<  8) | meas_readings.IR2L;
    if (IR2_raw & 0x800000) IR2_raw |= 0xFF000000;
    char    sign_IR2  = (IR2_raw < 0) ? '-' : '+';
    int32_t mV_IR2    = (int32_t)(((int64_t)IR2_raw * 7500LL) / 8388607LL);
    int32_t whole_IR2 = (mV_IR2 < 0 ? -mV_IR2 : mV_IR2) / 10;
    int32_t frac_IR2  = (mV_IR2 < 0 ? -mV_IR2 : mV_IR2) % 10;

    int16_t Vf_raw   = (int16_t)(((uint16_t)meas_readings.VFH << 8) | meas_readings.VFL);
    int32_t mV_Vf    = (int32_t)(((int64_t)Vf_raw * 15000LL) / 32767LL) + 14000;
    char    sign_Vf  = (mV_Vf < 0) ? '-' : '+';
    int32_t whole_Vf = (mV_Vf < 0 ? -mV_Vf : mV_Vf) / 10;
    int32_t frac_Vf  = (mV_Vf < 0 ? -mV_Vf : mV_Vf) % 10;

    sprintf(dsp_buff1, "IR1 = %c%07ld.%01ld mV ", sign_IR1, whole_IR1, frac_IR1);
    sprintf(dsp_buff2, "IR2 = %c%07ld.%01ld mV ", sign_IR2, whole_IR2, frac_IR2);
    sprintf(dsp_buff3, "VF  = %c%07ld.%01ld mV ", sign_Vf,  whole_Vf,  frac_Vf);
    sprintf(dsp_buff4, "                    ");
}

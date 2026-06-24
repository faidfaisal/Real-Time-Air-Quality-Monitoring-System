/*
 * ak9723.h  —  AK9723AJ sensor driver header (Task 2)
 *
 * Authors: Faid Faisal 
 * Created: 4/22/2026
 */

#ifndef AK9723_H
#define AK9723_H

#include <stdint.h>

#define AK9723AJ_ADDR 0x65   // 7-bit I²C address
#define OVCUR_DET     0x04   // Overcurrent detection flag in ST1

typedef struct {
    uint8_t ST1;              // Status register
    uint8_t IR1L, IR1M, IR1H;
    uint8_t IR2L, IR2M, IR2H;
    uint8_t TMPL, TMPH;
    uint8_t VFL,  VFH;
} data_read_t;

extern volatile data_read_t meas_readings;

void TWI0_AK9723_init(void);
void AK9723_TWI0_init(void);
void TWI0_AK9723_byte_write(uint8_t saddr, uint8_t raddr, uint8_t data);
void TWI0_AK9723_block_read(uint8_t saddr, uint8_t raddr);
void error_check(void);
void convert_store_vals(void);

#endif /* AK9723_H */

/*
 * task2fsm.c  —  Table-driven FSM Command Parser (Task 2)
 *
 * Created: April 28, 2026
 * Authors: Faid Faisal & Melchai Mathew
 *
 * Description:
 * Implements a table-driven FSM that parses Cn=hh<CR> commands received
 * over USART3.  On a valid command the value is written to the corresponding
 * AK9723AJ control register via I²C and a local shadow copy is updated.
 *
 * Character classes:
 *   CC_C   – 'C' or 'c'
 *   CC_DIG – '1'-'9'   (register digit, checked before CC_HEX)
 *   CC_EQ  – '='
 *   CC_HEX – 0-9, A-F, a-f
 *   CC_CR  – '\r'
 *   CC_OTHER
 */

#include <avr/io.h>
#include <stdint.h>
#include "fsm.h"
#include "ak9723.h"

/* ── State & character-class enumerations ───────────────────────────────── */
typedef enum {
    STATE_IDLE = 0,
    STATE_C    = 1,
    STATE_REG  = 2,
    STATE_EQUAL= 3,
    STATE_VAL1 = 4,
    STATE_VAL2 = 5,
    NUM_STATES = 6
} state_t;

typedef enum {
    CC_C = 0, CC_DIG, CC_EQ, CC_HEX, CC_CR, CC_OTHER,
    NUM_CC = 6
} char_class_t;

/* ── Transition table: next_state[current][input] ───────────────────────── */
static const state_t next_state[NUM_STATES][NUM_CC] = {
/*              CC_C       CC_DIG     CC_EQ       CC_HEX     CC_CR      CC_OTHER */
/* IDLE  */ { STATE_C,  STATE_IDLE, STATE_IDLE, STATE_IDLE, STATE_IDLE, STATE_IDLE },
/* C     */ { STATE_IDLE,STATE_REG, STATE_IDLE, STATE_IDLE, STATE_IDLE, STATE_IDLE },
/* REG   */ { STATE_IDLE,STATE_IDLE,STATE_EQUAL,STATE_IDLE, STATE_IDLE, STATE_IDLE },
/* EQUAL */ { STATE_IDLE,STATE_IDLE,STATE_IDLE, STATE_VAL1, STATE_IDLE, STATE_IDLE },
/* VAL1  */ { STATE_IDLE,STATE_IDLE,STATE_IDLE, STATE_VAL2, STATE_IDLE, STATE_IDLE },
/* VAL2  */ { STATE_IDLE,STATE_IDLE,STATE_IDLE, STATE_IDLE, STATE_IDLE, STATE_IDLE },
};

/* ── Module state ───────────────────────────────────────────────────────── */
static volatile state_t current_state = STATE_IDLE;
static volatile uint8_t reg_num = 0;
static volatile uint8_t value   = 0;

volatile uint8_t cntl_regs[10] = {0};  // shadow copy of CNTL1..CNTL9

/* ── Helpers ────────────────────────────────────────────────────────────── */
static char_class_t classify(uint8_t c)
{
    if (c == 'C' || c == 'c')                           return CC_C;
    if (c >= '1' && c <= '9')                           return CC_DIG;
    if (c == '=')                                        return CC_EQ;
    if ((c >= '0' && c <= '9') ||
        (c >= 'A' && c <= 'F') ||
        (c >= 'a' && c <= 'f'))                          return CC_HEX;
    if (c == '\r')                                       return CC_CR;
    return CC_OTHER;
}

static uint8_t hex_to_val(uint8_t c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return 0;
}

static void execute_command(uint8_t reg, uint8_t val)
{
    uint8_t reg_addr = CNTL_BASE_ADDR + (reg - 1);
    TWI0_AK9723_byte_write(AK9723AJ_ADDR, reg_addr, val);
    cntl_regs[reg] = val;
}

/* ── Public API ─────────────────────────────────────────────────────────── */
void fsm_init(void)
{
    current_state = STATE_IDLE;
    reg_num = value = 0;
    for (uint8_t i = 0; i < 10; i++) cntl_regs[i] = 0;
}

void fsm_parse(uint8_t c)
{
    char_class_t cc = classify(c);

    /* Side-effects executed BEFORE the state transition */
    if (current_state == STATE_C     && cc == CC_DIG) reg_num = c - '0';
    if (current_state == STATE_EQUAL && cc == CC_HEX) value   = hex_to_val(c) << 4;
    if (current_state == STATE_VAL1  && cc == CC_HEX) value  |= hex_to_val(c);
    if (current_state == STATE_VAL2  && cc == CC_CR)  execute_command(reg_num, value);

    current_state = next_state[current_state][cc];
}

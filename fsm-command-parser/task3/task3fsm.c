/*
 * task3fsm.c  —  Table-driven FSM with Page-Toggle Command (Task 3)
 *
 * Created: April 28, 2026
 * Authors: Faid Faisal 
 *
 * Description:
 * Extends the Task 2 FSM with a second command: P<CR> toggles the SerLCD
 * display between Page 0 (measured IR1/IR2/Vf values) and Page 1 (current
 * AK9723AJ control register settings C1–C9).
 *
 * New state: STATE_P — entered on 'P'/'p', executes toggle_page() on '\r'.
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
    STATE_P    = 6,
    NUM_STATES = 7
} state_t;

typedef enum {
    CC_C = 0, CC_DIG, CC_EQ, CC_HEX, CC_CR, CC_P, CC_OTHER,
    NUM_CC = 7
} char_class_t;

/* ── Transition table ───────────────────────────────────────────────────── */
static const state_t next_state[NUM_STATES][NUM_CC] = {
/*              CC_C       CC_DIG     CC_EQ       CC_HEX     CC_CR      CC_P       CC_OTHER */
/* IDLE  */ { STATE_C,  STATE_IDLE, STATE_IDLE, STATE_IDLE, STATE_IDLE, STATE_P,  STATE_IDLE },
/* C     */ { STATE_IDLE,STATE_REG, STATE_IDLE, STATE_IDLE, STATE_IDLE, STATE_IDLE,STATE_IDLE },
/* REG   */ { STATE_IDLE,STATE_IDLE,STATE_EQUAL,STATE_IDLE, STATE_IDLE, STATE_IDLE,STATE_IDLE },
/* EQUAL */ { STATE_IDLE,STATE_IDLE,STATE_IDLE, STATE_VAL1, STATE_IDLE, STATE_IDLE,STATE_IDLE },
/* VAL1  */ { STATE_IDLE,STATE_IDLE,STATE_IDLE, STATE_VAL2, STATE_IDLE, STATE_IDLE,STATE_IDLE },
/* VAL2  */ { STATE_IDLE,STATE_IDLE,STATE_IDLE, STATE_IDLE, STATE_IDLE, STATE_IDLE,STATE_IDLE },
/* P     */ { STATE_IDLE,STATE_IDLE,STATE_IDLE, STATE_IDLE, STATE_IDLE, STATE_IDLE,STATE_IDLE },
};

/* ── Module state ───────────────────────────────────────────────────────── */
static volatile state_t current_state = STATE_IDLE;
static volatile uint8_t reg_num = 0;
static volatile uint8_t value   = 0;

volatile uint8_t cntl_regs[10] = {0};
volatile uint8_t pg_num        = 0;   // 0 = measurements, 1 = settings

/* ── Helpers ────────────────────────────────────────────────────────────── */
static char_class_t classify(uint8_t c)
{
    if (c == 'C' || c == 'c')                           return CC_C;
    if (c == 'P' || c == 'p')                           return CC_P;
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
void toggle_page(void)
{
    pg_num ^= 1;
}

void fsm_init(void)
{
    current_state = STATE_IDLE;
    reg_num = value = pg_num = 0;
    for (uint8_t i = 0; i < 10; i++) cntl_regs[i] = 0;
}

void fsm_parse(uint8_t c)
{
    char_class_t cc = classify(c);

    if (current_state == STATE_C     && cc == CC_DIG) reg_num = c - '0';
    if (current_state == STATE_EQUAL && cc == CC_HEX) value   = hex_to_val(c) << 4;
    if (current_state == STATE_VAL1  && cc == CC_HEX) value  |= hex_to_val(c);
    if (current_state == STATE_VAL2  && cc == CC_CR)  execute_command(reg_num, value);
    if (current_state == STATE_P     && cc == CC_CR)  toggle_page();

    current_state = next_state[current_state][cc];
}

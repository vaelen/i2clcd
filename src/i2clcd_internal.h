/*
 * Copyright (c) 2026 Andrew C. Young
 * SPDX-License-Identifier: MIT
 *
 * i2clcd_internal.h - Internal definitions for libi2clcd
 */

#ifndef I2CLCD_INTERNAL_H
#define I2CLCD_INTERNAL_H

#include <stdint.h>
#include <stdbool.h>
#include "i2clcd.h"

/*---------------------------------------------------------------------------
 * PCF8574 Pin Mapping for HD44780
 * Common backpack configuration (active high enable, active high backlight)
 *---------------------------------------------------------------------------*/

#define PCF8574_PIN_RS              (1 << 0)  /* P0: Register Select */
#define PCF8574_PIN_RW              (1 << 1)  /* P1: Read/Write (tie low) */
#define PCF8574_PIN_EN              (1 << 2)  /* P2: Enable */
#define PCF8574_PIN_BL              (1 << 3)  /* P3: Backlight */
#define PCF8574_PIN_D4              (1 << 4)  /* P4: Data bit 4 */
#define PCF8574_PIN_D5              (1 << 5)  /* P5: Data bit 5 */
#define PCF8574_PIN_D6              (1 << 6)  /* P6: Data bit 6 */
#define PCF8574_PIN_D7              (1 << 7)  /* P7: Data bit 7 */

/* Data nibble mask (upper 4 bits of PCF8574) */
#define PCF8574_DATA_MASK           0xF0

/*---------------------------------------------------------------------------
 * LCD Context Structure (internal state)
 *---------------------------------------------------------------------------*/

struct i2clcd_ctx {
    int      fd;           /* I2C file descriptor */
    uint8_t  i2c_addr;     /* PCF8574 I2C address */
    uint8_t  cols;         /* Number of columns */
    uint8_t  rows;         /* Number of rows */
    uint8_t  display_ctrl; /* Display control register state */
    uint8_t  entry_mode;   /* Entry mode register state */
    bool     backlight;    /* Current backlight state */
    uint8_t  line_addr[4]; /* DDRAM address for each line */
};

/*---------------------------------------------------------------------------
 * Internal Function Prototypes
 *---------------------------------------------------------------------------*/

/* Low-level I2C write */
int i2clcd_i2c_write_byte(i2clcd_t *ctx, uint8_t byte);

/* Write a nibble to the LCD (4-bit mode) */
int i2clcd_write_nibble(i2clcd_t *ctx, uint8_t nibble, bool rs);

/* Write a full byte to the LCD (two nibbles) */
int i2clcd_write_byte(i2clcd_t *ctx, uint8_t byte, bool rs);

/* Send command to LCD */
int i2clcd_command(i2clcd_t *ctx, uint8_t cmd);

/* Send data to LCD */
int i2clcd_data(i2clcd_t *ctx, uint8_t data);

/* Update display control register */
int i2clcd_update_display_ctrl(i2clcd_t *ctx);

/* Microsecond delay (portable) */
void i2clcd_delay_us(unsigned int us);

/* Millisecond delay */
void i2clcd_delay_ms(unsigned int ms);

#endif /* I2CLCD_INTERNAL_H */

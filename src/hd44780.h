/*
 * Copyright (c) 2026 Andrew C. Young
 * SPDX-License-Identifier: MIT
 *
 * hd44780.h - HD44780 LCD controller command definitions
 */

#ifndef HD44780_H
#define HD44780_H

/*---------------------------------------------------------------------------
 * HD44780 Command Set
 *---------------------------------------------------------------------------*/

/* Commands */
#define HD44780_CMD_CLEAR           0x01  /* Clear display */
#define HD44780_CMD_HOME            0x02  /* Return home */
#define HD44780_CMD_ENTRY_MODE      0x04  /* Entry mode set */
#define HD44780_CMD_DISPLAY_CTRL    0x08  /* Display on/off control */
#define HD44780_CMD_SHIFT           0x10  /* Cursor/display shift */
#define HD44780_CMD_FUNCTION_SET    0x20  /* Function set */
#define HD44780_CMD_SET_CGRAM       0x40  /* Set CGRAM address */
#define HD44780_CMD_SET_DDRAM       0x80  /* Set DDRAM address */

/* Entry mode flags */
#define HD44780_ENTRY_INC           0x02  /* Increment cursor */
#define HD44780_ENTRY_SHIFT         0x01  /* Shift display */

/* Display control flags */
#define HD44780_DISPLAY_ON          0x04  /* Display on */
#define HD44780_CURSOR_ON           0x02  /* Cursor on */
#define HD44780_BLINK_ON            0x01  /* Blink on */

/* Function set flags */
#define HD44780_8BIT_MODE           0x10  /* 8-bit interface */
#define HD44780_4BIT_MODE           0x00  /* 4-bit interface */
#define HD44780_2LINE               0x08  /* 2-line display */
#define HD44780_1LINE               0x00  /* 1-line display */
#define HD44780_5X10_DOTS           0x04  /* 5x10 dot font */
#define HD44780_5X8_DOTS            0x00  /* 5x8 dot font */

/*---------------------------------------------------------------------------
 * DDRAM Line Addresses
 * Note: 20x4 displays have non-contiguous line addresses
 *---------------------------------------------------------------------------*/

#define HD44780_LINE0_ADDR          0x00
#define HD44780_LINE1_ADDR          0x40
#define HD44780_LINE2_ADDR          0x14  /* For 20x4 displays */
#define HD44780_LINE3_ADDR          0x54  /* For 20x4 displays */

/*---------------------------------------------------------------------------
 * Timing Constants (in microseconds)
 *---------------------------------------------------------------------------*/

#define HD44780_DELAY_CLEAR_US      1600  /* Clear/home command */
#define HD44780_DELAY_CMD_US        50    /* Most commands */
#define HD44780_DELAY_ENABLE_US     1     /* Enable pulse width */
#define HD44780_DELAY_INIT_MS       50    /* Power-on init delay */

#endif /* HD44780_H */

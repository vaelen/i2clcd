/*
 * Copyright (c) 2026 Andrew C. Young
 * SPDX-License-Identifier: MIT
 *
 * i2clcd.c - HD44780 LCD control via PCF8574 I2C backpack
 */

#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#include "i2clcd.h"
#include "i2clcd_internal.h"
#include "hd44780.h"

/*---------------------------------------------------------------------------
 * Error Strings
 *---------------------------------------------------------------------------*/

static const char *error_strings[] = {
    "Success",
    "Failed to open I2C device",
    "ioctl failed",
    "I2C write failed",
    "Invalid argument",
    "LCD not initialized",
    "Value out of range",
};

const char *i2clcd_strerror(i2clcd_err_t err)
{
    int idx = -err;
    if (idx < 0 || idx >= (int)(sizeof(error_strings) / sizeof(error_strings[0]))) {
        return "Unknown error";
    }
    return error_strings[idx];
}

/*---------------------------------------------------------------------------
 * Timing Functions
 *---------------------------------------------------------------------------*/

void i2clcd_delay_us(unsigned int us)
{
    struct timespec ts;

    ts.tv_sec = us / 1000000;
    ts.tv_nsec = (us % 1000000) * 1000;

    while (nanosleep(&ts, &ts) == -1 && errno == EINTR) {
        /* Retry if interrupted by signal */
    }
}

void i2clcd_delay_ms(unsigned int ms)
{
    i2clcd_delay_us(ms * 1000);
}

/*---------------------------------------------------------------------------
 * Low-Level I2C Functions
 *---------------------------------------------------------------------------*/

int i2clcd_i2c_write_byte(i2clcd_t *ctx, uint8_t byte)
{
    ssize_t ret;

    ret = write(ctx->fd, &byte, 1);
    if (ret != 1) {
        return -1;
    }

    return 0;
}

/*---------------------------------------------------------------------------
 * LCD Write Functions (4-bit mode)
 *---------------------------------------------------------------------------*/

int i2clcd_write_nibble(i2clcd_t *ctx, uint8_t nibble, bool rs)
{
    uint8_t data;
    int ret;

    /* Build the byte to send:
     * - Upper nibble is the data
     * - RS bit set based on command/data
     * - Backlight bit preserved
     */
    data = (nibble & PCF8574_DATA_MASK);
    if (rs) {
        data |= PCF8574_PIN_RS;
    }
    if (ctx->backlight) {
        data |= PCF8574_PIN_BL;
    }

    /* Write data with Enable HIGH */
    ret = i2clcd_i2c_write_byte(ctx, data | PCF8574_PIN_EN);
    if (ret < 0) {
        return ret;
    }

    i2clcd_delay_us(HD44780_DELAY_ENABLE_US);

    /* Write data with Enable LOW (falling edge latches data) */
    ret = i2clcd_i2c_write_byte(ctx, data);
    if (ret < 0) {
        return ret;
    }

    i2clcd_delay_us(HD44780_DELAY_CMD_US);

    return 0;
}

int i2clcd_write_byte(i2clcd_t *ctx, uint8_t byte, bool rs)
{
    int ret;

    /* High nibble first */
    ret = i2clcd_write_nibble(ctx, byte & 0xF0, rs);
    if (ret < 0) {
        return ret;
    }

    /* Low nibble (shifted to upper position) */
    ret = i2clcd_write_nibble(ctx, (byte << 4) & 0xF0, rs);
    return ret;
}

int i2clcd_command(i2clcd_t *ctx, uint8_t cmd)
{
    return i2clcd_write_byte(ctx, cmd, false);
}

int i2clcd_data(i2clcd_t *ctx, uint8_t data)
{
    return i2clcd_write_byte(ctx, data, true);
}

int i2clcd_update_display_ctrl(i2clcd_t *ctx)
{
    return i2clcd_command(ctx, HD44780_CMD_DISPLAY_CTRL | ctx->display_ctrl);
}

/*---------------------------------------------------------------------------
 * Initialization / Deinitialization
 *---------------------------------------------------------------------------*/

i2clcd_err_t i2clcd_open(const i2clcd_config_t *config, i2clcd_t **handle)
{
    i2clcd_t *ctx;

    /* Validate arguments */
    if (!config || !handle) {
        return I2CLCD_ERR_INVALID_ARG;
    }

    /* Allocate context */
    ctx = calloc(1, sizeof(*ctx));
    if (!ctx) {
        return I2CLCD_ERR_OPEN;
    }

    /* Store configuration */
    ctx->i2c_addr = config->i2c_addr;
    ctx->backlight = config->backlight;

    /* Set dimensions based on size preset */
    switch (config->size) {
    case I2CLCD_16X2:
        ctx->cols = 16;
        ctx->rows = 2;
        break;
    case I2CLCD_20X4:
        ctx->cols = 20;
        ctx->rows = 4;
        break;
    case I2CLCD_CUSTOM:
        ctx->cols = config->cols;
        ctx->rows = config->rows;
        break;
    default:
        free(ctx);
        return I2CLCD_ERR_INVALID_ARG;
    }

    /* Set line addresses (DDRAM offsets) */
    ctx->line_addr[0] = HD44780_LINE0_ADDR;
    ctx->line_addr[1] = HD44780_LINE1_ADDR;
    ctx->line_addr[2] = HD44780_LINE2_ADDR;
    ctx->line_addr[3] = HD44780_LINE3_ADDR;

    /* Open I2C device */
    ctx->fd = open(config->i2c_device, O_RDWR);
    if (ctx->fd < 0) {
        free(ctx);
        return I2CLCD_ERR_OPEN;
    }

    /* Set I2C slave address */
    if (ioctl(ctx->fd, I2C_SLAVE, ctx->i2c_addr) < 0) {
        close(ctx->fd);
        free(ctx);
        return I2CLCD_ERR_IOCTL;
    }

    /* Set default state for already-initialized display */
    ctx->display_ctrl = HD44780_DISPLAY_ON;
    ctx->entry_mode = HD44780_ENTRY_INC;

    *handle = ctx;
    return I2CLCD_OK;
}

i2clcd_err_t i2clcd_init(const i2clcd_config_t *config, i2clcd_t **handle)
{
    i2clcd_err_t err;
    i2clcd_t *ctx;

    /* Open I2C connection first */
    err = i2clcd_open(config, handle);
    if (err != I2CLCD_OK) {
        return err;
    }

    ctx = *handle;

    /*-----------------------------------------------------------------------
     * HD44780 Initialization for 4-bit mode (from datasheet)
     * This sequence is required even if the display was already in 4-bit mode
     *-----------------------------------------------------------------------*/

    /* Wait >40ms after power-on */
    i2clcd_delay_ms(HD44780_DELAY_INIT_MS);

    /* Start with backlight state, all control pins low */
    i2clcd_i2c_write_byte(ctx, ctx->backlight ? PCF8574_PIN_BL : 0);
    i2clcd_delay_ms(1);

    /*
     * Step 1: Send 0x30 (Function Set, 8-bit) three times
     * This ensures the controller is in a known state regardless of
     * whether it was in 4-bit or 8-bit mode before
     */
    i2clcd_write_nibble(ctx, 0x30, false);  /* 8-bit mode */
    i2clcd_delay_ms(5);                      /* Wait >4.1ms */

    i2clcd_write_nibble(ctx, 0x30, false);  /* 8-bit mode again */
    i2clcd_delay_us(150);                    /* Wait >100us */

    i2clcd_write_nibble(ctx, 0x30, false);  /* 8-bit mode third time */
    i2clcd_delay_us(150);

    /* Step 2: Set 4-bit mode */
    i2clcd_write_nibble(ctx, 0x20, false);
    i2clcd_delay_us(HD44780_DELAY_CMD_US);

    /* Now we can use normal byte-write functions */

    /* Step 3: Function set (4-bit, 2-line, 5x8 dots) */
    i2clcd_command(ctx, HD44780_CMD_FUNCTION_SET |
                        HD44780_4BIT_MODE |
                        HD44780_2LINE |
                        HD44780_5X8_DOTS);

    /* Step 4: Display off */
    ctx->display_ctrl = 0;
    i2clcd_command(ctx, HD44780_CMD_DISPLAY_CTRL);

    /* Step 5: Entry mode set (increment, no shift) */
    ctx->entry_mode = HD44780_ENTRY_INC;
    i2clcd_command(ctx, HD44780_CMD_ENTRY_MODE | ctx->entry_mode);

    /* Step 6: Display on (cursor and blink off by default) */
    ctx->display_ctrl = HD44780_DISPLAY_ON;
    i2clcd_command(ctx, HD44780_CMD_DISPLAY_CTRL | ctx->display_ctrl);

    return I2CLCD_OK;
}

void i2clcd_deinit(i2clcd_t *handle)
{
    if (handle) {
        if (handle->fd >= 0) {
            close(handle->fd);
        }
        free(handle);
    }
}

/*---------------------------------------------------------------------------
 * Display Control
 *---------------------------------------------------------------------------*/

i2clcd_err_t i2clcd_clear(i2clcd_t *handle)
{
    if (!handle) {
        return I2CLCD_ERR_NOT_INIT;
    }

    if (i2clcd_command(handle, HD44780_CMD_CLEAR) < 0) {
        return I2CLCD_ERR_WRITE;
    }

    i2clcd_delay_us(HD44780_DELAY_CLEAR_US);
    return I2CLCD_OK;
}

i2clcd_err_t i2clcd_clear_line(i2clcd_t *handle, uint8_t line)
{
    i2clcd_err_t ret;
    uint8_t i;

    if (!handle) {
        return I2CLCD_ERR_NOT_INIT;
    }

    if (line >= handle->rows) {
        return I2CLCD_ERR_RANGE;
    }

    /* Position cursor at start of line */
    ret = i2clcd_set_cursor(handle, 0, line);
    if (ret != I2CLCD_OK) {
        return ret;
    }

    /* Fill line with spaces */
    for (i = 0; i < handle->cols; i++) {
        if (i2clcd_data(handle, ' ') < 0) {
            return I2CLCD_ERR_WRITE;
        }
    }

    return I2CLCD_OK;
}

i2clcd_err_t i2clcd_home(i2clcd_t *handle)
{
    if (!handle) {
        return I2CLCD_ERR_NOT_INIT;
    }

    if (i2clcd_command(handle, HD44780_CMD_HOME) < 0) {
        return I2CLCD_ERR_WRITE;
    }

    i2clcd_delay_us(HD44780_DELAY_CLEAR_US);
    return I2CLCD_OK;
}

i2clcd_err_t i2clcd_display(i2clcd_t *handle, bool on)
{
    if (!handle) {
        return I2CLCD_ERR_NOT_INIT;
    }

    if (on) {
        handle->display_ctrl |= HD44780_DISPLAY_ON;
    } else {
        handle->display_ctrl &= ~HD44780_DISPLAY_ON;
    }

    if (i2clcd_update_display_ctrl(handle) < 0) {
        return I2CLCD_ERR_WRITE;
    }

    return I2CLCD_OK;
}

/*---------------------------------------------------------------------------
 * Cursor Control
 *---------------------------------------------------------------------------*/

i2clcd_err_t i2clcd_set_cursor(i2clcd_t *handle, uint8_t col, uint8_t row)
{
    uint8_t addr;

    if (!handle) {
        return I2CLCD_ERR_NOT_INIT;
    }

    if (row >= handle->rows || col >= handle->cols) {
        return I2CLCD_ERR_RANGE;
    }

    /* Calculate DDRAM address */
    addr = handle->line_addr[row] + col;

    /* Send Set DDRAM Address command */
    if (i2clcd_command(handle, HD44780_CMD_SET_DDRAM | addr) < 0) {
        return I2CLCD_ERR_WRITE;
    }

    return I2CLCD_OK;
}

i2clcd_err_t i2clcd_cursor(i2clcd_t *handle, bool visible)
{
    if (!handle) {
        return I2CLCD_ERR_NOT_INIT;
    }

    if (visible) {
        handle->display_ctrl |= HD44780_CURSOR_ON;
    } else {
        handle->display_ctrl &= ~HD44780_CURSOR_ON;
    }

    if (i2clcd_update_display_ctrl(handle) < 0) {
        return I2CLCD_ERR_WRITE;
    }

    return I2CLCD_OK;
}

i2clcd_err_t i2clcd_blink(i2clcd_t *handle, bool blink)
{
    if (!handle) {
        return I2CLCD_ERR_NOT_INIT;
    }

    if (blink) {
        handle->display_ctrl |= HD44780_BLINK_ON;
    } else {
        handle->display_ctrl &= ~HD44780_BLINK_ON;
    }

    if (i2clcd_update_display_ctrl(handle) < 0) {
        return I2CLCD_ERR_WRITE;
    }

    return I2CLCD_OK;
}

/*---------------------------------------------------------------------------
 * Writing Text
 *---------------------------------------------------------------------------*/

i2clcd_err_t i2clcd_putc(i2clcd_t *handle, char c)
{
    if (!handle) {
        return I2CLCD_ERR_NOT_INIT;
    }

    if (i2clcd_data(handle, (uint8_t)c) < 0) {
        return I2CLCD_ERR_WRITE;
    }

    return I2CLCD_OK;
}

i2clcd_err_t i2clcd_puts(i2clcd_t *handle, const char *str)
{
    if (!handle) {
        return I2CLCD_ERR_NOT_INIT;
    }

    if (!str) {
        return I2CLCD_ERR_INVALID_ARG;
    }

    while (*str) {
        if (i2clcd_data(handle, (uint8_t)*str++) < 0) {
            return I2CLCD_ERR_WRITE;
        }
    }

    return I2CLCD_OK;
}

i2clcd_err_t i2clcd_printf(i2clcd_t *handle, const char *fmt, ...)
{
    char buf[128];
    va_list ap;

    if (!handle) {
        return I2CLCD_ERR_NOT_INIT;
    }

    if (!fmt) {
        return I2CLCD_ERR_INVALID_ARG;
    }

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    return i2clcd_puts(handle, buf);
}

i2clcd_err_t i2clcd_set_line(i2clcd_t *handle, uint8_t line, const char *text)
{
    i2clcd_err_t ret;
    size_t len, i;

    if (!handle) {
        return I2CLCD_ERR_NOT_INIT;
    }

    if (!text) {
        return I2CLCD_ERR_INVALID_ARG;
    }

    if (line >= handle->rows) {
        return I2CLCD_ERR_RANGE;
    }

    /* Position cursor at start of line */
    ret = i2clcd_set_cursor(handle, 0, line);
    if (ret != I2CLCD_OK) {
        return ret;
    }

    /* Write text, padding with spaces if shorter than line width */
    len = strlen(text);
    for (i = 0; i < handle->cols; i++) {
        char c = (i < len) ? text[i] : ' ';
        if (i2clcd_data(handle, (uint8_t)c) < 0) {
            return I2CLCD_ERR_WRITE;
        }
    }

    return I2CLCD_OK;
}

/*---------------------------------------------------------------------------
 * Backlight Control
 *---------------------------------------------------------------------------*/

i2clcd_err_t i2clcd_backlight(i2clcd_t *handle, bool on)
{
    if (!handle) {
        return I2CLCD_ERR_NOT_INIT;
    }

    handle->backlight = on;

    /* Send a no-op I2C write to update backlight state */
    if (i2clcd_i2c_write_byte(handle, on ? PCF8574_PIN_BL : 0) < 0) {
        return I2CLCD_ERR_WRITE;
    }

    return I2CLCD_OK;
}

i2clcd_err_t i2clcd_backlight_get(i2clcd_t *handle, bool *on)
{
    if (!handle) {
        return I2CLCD_ERR_NOT_INIT;
    }

    if (!on) {
        return I2CLCD_ERR_INVALID_ARG;
    }

    *on = handle->backlight;
    return I2CLCD_OK;
}

/*---------------------------------------------------------------------------
 * Custom Characters (CGRAM)
 *---------------------------------------------------------------------------*/

i2clcd_err_t i2clcd_create_char(i2clcd_t *handle, uint8_t location,
                                const uint8_t charmap[8])
{
    int i;

    if (!handle) {
        return I2CLCD_ERR_NOT_INIT;
    }

    if (!charmap) {
        return I2CLCD_ERR_INVALID_ARG;
    }

    if (location > 7) {
        return I2CLCD_ERR_RANGE;
    }

    /* Set CGRAM address */
    if (i2clcd_command(handle, HD44780_CMD_SET_CGRAM | (location << 3)) < 0) {
        return I2CLCD_ERR_WRITE;
    }

    /* Write character pattern (8 bytes) */
    for (i = 0; i < 8; i++) {
        if (i2clcd_data(handle, charmap[i]) < 0) {
            return I2CLCD_ERR_WRITE;
        }
    }

    /* Return to DDRAM mode */
    if (i2clcd_command(handle, HD44780_CMD_SET_DDRAM) < 0) {
        return I2CLCD_ERR_WRITE;
    }

    return I2CLCD_OK;
}

/*---------------------------------------------------------------------------
 * Utility Functions
 *---------------------------------------------------------------------------*/

i2clcd_err_t i2clcd_get_size(i2clcd_t *handle, uint8_t *cols, uint8_t *rows)
{
    if (!handle) {
        return I2CLCD_ERR_NOT_INIT;
    }

    if (cols) {
        *cols = handle->cols;
    }

    if (rows) {
        *rows = handle->rows;
    }

    return I2CLCD_OK;
}

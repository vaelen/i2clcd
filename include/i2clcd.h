/*
 * Copyright (c) 2026 Andrew C. Young
 * SPDX-License-Identifier: MIT
 *
 * i2clcd.h - Public API for HD44780 LCD control via PCF8574 I2C backpack
 */

#ifndef I2CLCD_H
#define I2CLCD_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Version information */
#define I2CLCD_VERSION_MAJOR 1
#define I2CLCD_VERSION_MINOR 0
#define I2CLCD_VERSION_PATCH 0

/* Error codes */
typedef enum {
    I2CLCD_OK              =  0,   /* Success */
    I2CLCD_ERR_OPEN        = -1,   /* Failed to open I2C device */
    I2CLCD_ERR_IOCTL       = -2,   /* ioctl failed (address set) */
    I2CLCD_ERR_WRITE       = -3,   /* Write to I2C failed */
    I2CLCD_ERR_INVALID_ARG = -4,   /* Invalid argument */
    I2CLCD_ERR_NOT_INIT    = -5,   /* LCD not initialized */
    I2CLCD_ERR_RANGE       = -6,   /* Value out of range */
} i2clcd_err_t;

/* LCD size presets */
typedef enum {
    I2CLCD_16X2 = 0,   /* 16 columns, 2 rows (1602A) */
    I2CLCD_20X4 = 1,   /* 20 columns, 4 rows (2004A) */
    I2CLCD_CUSTOM,     /* Custom dimensions */
} i2clcd_size_t;

/* LCD configuration structure */
typedef struct {
    const char    *i2c_device;   /* e.g., "/dev/i2c-1" */
    uint8_t        i2c_addr;     /* PCF8574 address (0x20-0x27 or 0x38-0x3F) */
    i2clcd_size_t  size;         /* LCD size preset */
    uint8_t        cols;         /* Columns (used if size == I2CLCD_CUSTOM) */
    uint8_t        rows;         /* Rows (used if size == I2CLCD_CUSTOM) */
    bool           backlight;    /* Initial backlight state */
} i2clcd_config_t;

/* Opaque handle to LCD instance */
typedef struct i2clcd_ctx i2clcd_t;

/* Default configuration initializer */
#define I2CLCD_CONFIG_DEFAULT { \
    .i2c_device = "/dev/i2c-1", \
    .i2c_addr   = 0x27,         \
    .size       = I2CLCD_20X4,  \
    .cols       = 20,           \
    .rows       = 4,            \
    .backlight  = true,         \
}

/*---------------------------------------------------------------------------
 * Initialization / Deinitialization
 *---------------------------------------------------------------------------*/

/**
 * @brief Initialize LCD with given configuration
 * @param config Pointer to configuration structure
 * @param handle Pointer to receive LCD handle on success
 * @return I2CLCD_OK on success, negative error code on failure
 *
 * This performs the HD44780 initialization sequence but does not clear the
 * display. Call i2clcd_clear() explicitly if you want to clear the screen.
 */
i2clcd_err_t i2clcd_init(const i2clcd_config_t *config, i2clcd_t **handle);

/**
 * @brief Open connection to an already-initialized LCD
 * @param config Pointer to configuration structure
 * @param handle Pointer to receive LCD handle on success
 * @return I2CLCD_OK on success, negative error code on failure
 *
 * Opens I2C connection without LCD initialization. Use for commands
 * after the LCD has been initialized with i2clcd_init().
 */
i2clcd_err_t i2clcd_open(const i2clcd_config_t *config, i2clcd_t **handle);

/**
 * @brief Deinitialize LCD and free resources
 * @param handle LCD handle (may be NULL)
 */
void i2clcd_deinit(i2clcd_t *handle);

/**
 * @brief Get human-readable error string
 * @param err Error code
 * @return Static string describing the error
 */
const char *i2clcd_strerror(i2clcd_err_t err);

/*---------------------------------------------------------------------------
 * Display Control
 *---------------------------------------------------------------------------*/

/**
 * @brief Clear entire display
 * @param handle LCD handle
 * @return I2CLCD_OK on success, negative error code on failure
 */
i2clcd_err_t i2clcd_clear(i2clcd_t *handle);

/**
 * @brief Clear a specific line (fill with spaces)
 * @param handle LCD handle
 * @param line Line number (0-indexed)
 * @return I2CLCD_OK on success, negative error code on failure
 */
i2clcd_err_t i2clcd_clear_line(i2clcd_t *handle, uint8_t line);

/**
 * @brief Return cursor to home position (0, 0)
 * @param handle LCD handle
 * @return I2CLCD_OK on success, negative error code on failure
 */
i2clcd_err_t i2clcd_home(i2clcd_t *handle);

/**
 * @brief Turn display on or off (preserves content)
 * @param handle LCD handle
 * @param on true to turn on, false to turn off
 * @return I2CLCD_OK on success, negative error code on failure
 */
i2clcd_err_t i2clcd_display(i2clcd_t *handle, bool on);

/*---------------------------------------------------------------------------
 * Cursor Control
 *---------------------------------------------------------------------------*/

/**
 * @brief Set cursor position
 * @param handle LCD handle
 * @param col Column (0-indexed)
 * @param row Row (0-indexed)
 * @return I2CLCD_OK on success, negative error code on failure
 */
i2clcd_err_t i2clcd_set_cursor(i2clcd_t *handle, uint8_t col, uint8_t row);

/**
 * @brief Set cursor visibility
 * @param handle LCD handle
 * @param visible true to show cursor, false to hide
 * @return I2CLCD_OK on success, negative error code on failure
 */
i2clcd_err_t i2clcd_cursor(i2clcd_t *handle, bool visible);

/**
 * @brief Set cursor blink
 * @param handle LCD handle
 * @param blink true to enable blinking, false to disable
 * @return I2CLCD_OK on success, negative error code on failure
 */
i2clcd_err_t i2clcd_blink(i2clcd_t *handle, bool blink);

/*---------------------------------------------------------------------------
 * Writing Text
 *---------------------------------------------------------------------------*/

/**
 * @brief Write a single character at current cursor position
 * @param handle LCD handle
 * @param c Character to write
 * @return I2CLCD_OK on success, negative error code on failure
 */
i2clcd_err_t i2clcd_putc(i2clcd_t *handle, char c);

/**
 * @brief Write a string at current cursor position
 * @param handle LCD handle
 * @param str Null-terminated string
 * @return I2CLCD_OK on success, negative error code on failure
 */
i2clcd_err_t i2clcd_puts(i2clcd_t *handle, const char *str);

/**
 * @brief Write formatted string at current cursor position
 * @param handle LCD handle
 * @param fmt Format string (printf-style)
 * @param ... Format arguments
 * @return I2CLCD_OK on success, negative error code on failure
 */
i2clcd_err_t i2clcd_printf(i2clcd_t *handle, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));

/**
 * @brief Set an entire line's text (clears line first, truncates if too long)
 * @param handle LCD handle
 * @param line Line number (0-indexed)
 * @param text Text to display
 * @return I2CLCD_OK on success, negative error code on failure
 */
i2clcd_err_t i2clcd_set_line(i2clcd_t *handle, uint8_t line, const char *text);

/*---------------------------------------------------------------------------
 * Backlight Control
 *---------------------------------------------------------------------------*/

/**
 * @brief Set backlight state
 * @param handle LCD handle
 * @param on true for on, false for off
 * @return I2CLCD_OK on success, negative error code on failure
 */
i2clcd_err_t i2clcd_backlight(i2clcd_t *handle, bool on);

/**
 * @brief Get current backlight state
 * @param handle LCD handle
 * @param on Pointer to receive state
 * @return I2CLCD_OK on success, negative error code on failure
 */
i2clcd_err_t i2clcd_backlight_get(i2clcd_t *handle, bool *on);

/*---------------------------------------------------------------------------
 * Custom Characters (CGRAM)
 *---------------------------------------------------------------------------*/

/**
 * @brief Define a custom character
 * @param handle LCD handle
 * @param location Character slot (0-7)
 * @param charmap 8-byte array defining 5x8 pixel pattern
 * @return I2CLCD_OK on success, negative error code on failure
 */
i2clcd_err_t i2clcd_create_char(i2clcd_t *handle, uint8_t location,
                                const uint8_t charmap[8]);

/*---------------------------------------------------------------------------
 * Utility Functions
 *---------------------------------------------------------------------------*/

/**
 * @brief Get LCD dimensions
 * @param handle LCD handle
 * @param cols Pointer to receive column count (may be NULL)
 * @param rows Pointer to receive row count (may be NULL)
 * @return I2CLCD_OK on success, negative error code on failure
 */
i2clcd_err_t i2clcd_get_size(i2clcd_t *handle, uint8_t *cols, uint8_t *rows);

#ifdef __cplusplus
}
#endif

#endif /* I2CLCD_H */

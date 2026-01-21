/*
 * Copyright (c) 2026 Andrew C. Young
 * SPDX-License-Identifier: MIT
 *
 * demo.c - Demonstration of libi2clcd features
 */

#define _DEFAULT_SOURCE

#include <stdio.h>
#include <unistd.h>

#include "i2clcd.h"

int main(void)
{
    i2clcd_config_t config = I2CLCD_CONFIG_DEFAULT;
    i2clcd_t *lcd;
    i2clcd_err_t err;

    printf("LCD Demo - libi2clcd\n");
    printf("Initializing LCD on %s at address 0x%02X...\n",
           config.i2c_device, config.i2c_addr);

    err = i2clcd_init(&config, &lcd);
    if (err != I2CLCD_OK) {
        fprintf(stderr, "Failed to initialize LCD: %s\n",
                i2clcd_strerror(err));
        return 1;
    }

    printf("LCD initialized successfully!\n");

    /* Demo 1: Basic text display */
    printf("Demo 1: Basic text display\n");
    i2clcd_set_line(lcd, 0, "libi2clcd Demo");
    i2clcd_set_line(lcd, 1, "Hello, World!");
    sleep(2);

    /* Demo 2: Backlight control */
    printf("Demo 2: Backlight control\n");
    printf("  Backlight off...\n");
    i2clcd_backlight(lcd, false);
    sleep(1);
    printf("  Backlight on...\n");
    i2clcd_backlight(lcd, true);
    sleep(1);

    /* Demo 3: Clear and cursor positioning */
    printf("Demo 3: Cursor positioning\n");
    i2clcd_clear(lcd);
    i2clcd_set_cursor(lcd, 0, 0);
    i2clcd_puts(lcd, "Cursor test:");

    for (int i = 0; i < 10; i++) {
        i2clcd_set_cursor(lcd, i, 1);
        i2clcd_putc(lcd, '0' + i);
        usleep(200000);  /* 200ms delay */
    }
    sleep(1);

    /* Demo 4: Printf formatting */
    printf("Demo 4: Printf formatting\n");
    i2clcd_clear(lcd);
    for (int i = 0; i < 5; i++) {
        i2clcd_set_cursor(lcd, 0, 0);
        i2clcd_printf(lcd, "Counter: %d", i);
        i2clcd_set_cursor(lcd, 0, 1);
        i2clcd_printf(lcd, "Hex: 0x%02X", i * 16);
        sleep(1);
    }

    /* Demo 5: Cursor visibility */
    printf("Demo 5: Cursor visibility\n");
    i2clcd_clear(lcd);
    i2clcd_set_line(lcd, 0, "Cursor visible:");
    i2clcd_set_cursor(lcd, 0, 1);
    i2clcd_cursor(lcd, true);
    sleep(2);

    i2clcd_clear(lcd);
    i2clcd_set_line(lcd, 0, "Cursor blink:");
    i2clcd_set_cursor(lcd, 0, 1);
    i2clcd_blink(lcd, true);
    sleep(2);

    i2clcd_cursor(lcd, false);
    i2clcd_blink(lcd, false);

    /* Final message */
    i2clcd_clear(lcd);
    i2clcd_set_line(lcd, 0, "Demo complete!");
    i2clcd_set_line(lcd, 1, "Goodbye!");

    /* Cleanup */
    printf("Demo complete. Cleaning up...\n");
    i2clcd_deinit(lcd);

    return 0;
}

/*
 * Copyright (c) 2026 Andrew C. Young
 * SPDX-License-Identifier: MIT
 *
 * lcdctl.c - Command-line interface for HD44780 LCD via PCF8574
 */

#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <ctype.h>

#include "i2clcd.h"

#define DEFAULT_I2C_DEVICE  "/dev/i2c-1"
#define DEFAULT_I2C_ADDR    0x27

static void print_usage(const char *progname)
{
    fprintf(stderr,
        "Usage: %s [OPTIONS] COMMAND [ARGS]\n"
        "\n"
        "Control HD44780 LCD via PCF8574 I2C backpack\n"
        "\n"
        "Options:\n"
        "  -d, --device=DEV    I2C device (default: %s)\n"
        "  -a, --address=ADDR  I2C address in hex (default: 0x%02X)\n"
        "  -s, --size=SIZE     LCD size: 16x2 or 20x4 (default: 16x2)\n"
        "  -h, --help          Show this help message\n"
        "  -v, --version       Show version information\n"
        "\n"
        "Commands:\n"
        "  init                Initialize the LCD\n"
        "  clear               Clear the entire display\n"
        "  clear-line N        Clear line N (0-indexed)\n"
        "  line N TEXT         Set line N to TEXT\n"
        "  write TEXT          Write TEXT at current cursor position\n"
        "  cursor COL ROW      Set cursor position\n"
        "  backlight on|off    Turn backlight on or off\n"
        "  display on|off      Turn display on or off\n"
        "  cursor-show on|off  Show or hide cursor\n"
        "  cursor-blink on|off Enable or disable cursor blink\n"
        "  home                Return cursor to home position\n"
        "\n"
        "Examples:\n"
        "  %s init\n"
        "  %s line 0 \"Hello, World!\"\n"
        "  %s -a 0x3F -s 20x4 line 2 \"Line 3 text\"\n"
        "  %s backlight off\n"
        "\n",
        progname, DEFAULT_I2C_DEVICE, DEFAULT_I2C_ADDR,
        progname, progname, progname, progname);
}

static void print_version(void)
{
    printf("lcdctl version %d.%d.%d\n",
           I2CLCD_VERSION_MAJOR,
           I2CLCD_VERSION_MINOR,
           I2CLCD_VERSION_PATCH);
}

static int parse_bool(const char *str, bool *value)
{
    if (strcasecmp(str, "on") == 0 ||
        strcasecmp(str, "yes") == 0 ||
        strcasecmp(str, "1") == 0 ||
        strcasecmp(str, "true") == 0) {
        *value = true;
        return 0;
    }
    if (strcasecmp(str, "off") == 0 ||
        strcasecmp(str, "no") == 0 ||
        strcasecmp(str, "0") == 0 ||
        strcasecmp(str, "false") == 0) {
        *value = false;
        return 0;
    }
    return -1;
}

static int parse_size(const char *str, i2clcd_size_t *size)
{
    if (strcmp(str, "16x2") == 0 || strcmp(str, "1602") == 0) {
        *size = I2CLCD_16X2;
        return 0;
    }
    if (strcmp(str, "20x4") == 0 || strcmp(str, "2004") == 0) {
        *size = I2CLCD_20X4;
        return 0;
    }
    return -1;
}

int main(int argc, char *argv[])
{
    i2clcd_config_t config = I2CLCD_CONFIG_DEFAULT;
    i2clcd_t *lcd = NULL;
    i2clcd_err_t err;
    int ret = 0;

    static struct option long_options[] = {
        {"device",  required_argument, 0, 'd'},
        {"address", required_argument, 0, 'a'},
        {"size",    required_argument, 0, 's'},
        {"help",    no_argument,       0, 'h'},
        {"version", no_argument,       0, 'v'},
        {0, 0, 0, 0}
    };

    /* Parse options */
    int opt;
    while ((opt = getopt_long(argc, argv, "d:a:s:hv",
                              long_options, NULL)) != -1) {
        switch (opt) {
        case 'd':
            config.i2c_device = optarg;
            break;
        case 'a':
            config.i2c_addr = (uint8_t)strtol(optarg, NULL, 0);
            break;
        case 's':
            if (parse_size(optarg, &config.size) != 0) {
                fprintf(stderr, "Invalid size: %s\n", optarg);
                return 1;
            }
            break;
        case 'h':
            print_usage(argv[0]);
            return 0;
        case 'v':
            print_version();
            return 0;
        default:
            print_usage(argv[0]);
            return 1;
        }
    }

    /* Need at least one command */
    if (optind >= argc) {
        fprintf(stderr, "Error: No command specified\n");
        print_usage(argv[0]);
        return 1;
    }

    const char *cmd = argv[optind];
    int nargs = argc - optind - 1;
    char **args = &argv[optind + 1];

    /* Initialize or open LCD based on command */
    if (strcmp(cmd, "init") == 0) {
        err = i2clcd_init(&config, &lcd);
    } else {
        err = i2clcd_open(&config, &lcd);
    }
    if (err != I2CLCD_OK) {
        fprintf(stderr, "Error opening LCD: %s\n",
                i2clcd_strerror(err));
        return 1;
    }

    /* Process commands */
    if (strcmp(cmd, "init") == 0) {
        printf("LCD initialized successfully\n");

    } else if (strcmp(cmd, "clear") == 0) {
        err = i2clcd_clear(lcd);

    } else if (strcmp(cmd, "clear-line") == 0) {
        if (nargs < 1) {
            fprintf(stderr, "Error: clear-line requires line number\n");
            ret = 1;
            goto cleanup;
        }
        uint8_t line = (uint8_t)atoi(args[0]);
        err = i2clcd_clear_line(lcd, line);

    } else if (strcmp(cmd, "line") == 0) {
        if (nargs < 2) {
            fprintf(stderr, "Error: line requires line number and text\n");
            ret = 1;
            goto cleanup;
        }
        uint8_t line = (uint8_t)atoi(args[0]);
        err = i2clcd_set_line(lcd, line, args[1]);

    } else if (strcmp(cmd, "write") == 0) {
        if (nargs < 1) {
            fprintf(stderr, "Error: write requires text argument\n");
            ret = 1;
            goto cleanup;
        }
        err = i2clcd_puts(lcd, args[0]);

    } else if (strcmp(cmd, "cursor") == 0) {
        if (nargs < 2) {
            fprintf(stderr, "Error: cursor requires column and row\n");
            ret = 1;
            goto cleanup;
        }
        uint8_t col = (uint8_t)atoi(args[0]);
        uint8_t row = (uint8_t)atoi(args[1]);
        err = i2clcd_set_cursor(lcd, col, row);

    } else if (strcmp(cmd, "backlight") == 0) {
        if (nargs < 1) {
            fprintf(stderr, "Error: backlight requires on/off\n");
            ret = 1;
            goto cleanup;
        }
        bool on;
        if (parse_bool(args[0], &on) != 0) {
            fprintf(stderr, "Error: invalid backlight value: %s\n", args[0]);
            ret = 1;
            goto cleanup;
        }
        err = i2clcd_backlight(lcd, on);

    } else if (strcmp(cmd, "display") == 0) {
        if (nargs < 1) {
            fprintf(stderr, "Error: display requires on/off\n");
            ret = 1;
            goto cleanup;
        }
        bool on;
        if (parse_bool(args[0], &on) != 0) {
            fprintf(stderr, "Error: invalid display value: %s\n", args[0]);
            ret = 1;
            goto cleanup;
        }
        err = i2clcd_display(lcd, on);

    } else if (strcmp(cmd, "cursor-show") == 0) {
        if (nargs < 1) {
            fprintf(stderr, "Error: cursor-show requires on/off\n");
            ret = 1;
            goto cleanup;
        }
        bool on;
        if (parse_bool(args[0], &on) != 0) {
            fprintf(stderr, "Error: invalid cursor-show value: %s\n", args[0]);
            ret = 1;
            goto cleanup;
        }
        err = i2clcd_cursor(lcd, on);

    } else if (strcmp(cmd, "cursor-blink") == 0) {
        if (nargs < 1) {
            fprintf(stderr, "Error: cursor-blink requires on/off\n");
            ret = 1;
            goto cleanup;
        }
        bool on;
        if (parse_bool(args[0], &on) != 0) {
            fprintf(stderr, "Error: invalid cursor-blink value: %s\n", args[0]);
            ret = 1;
            goto cleanup;
        }
        err = i2clcd_blink(lcd, on);

    } else if (strcmp(cmd, "home") == 0) {
        err = i2clcd_home(lcd);

    } else {
        fprintf(stderr, "Error: Unknown command: %s\n", cmd);
        ret = 1;
        goto cleanup;
    }

    if (err != I2CLCD_OK) {
        fprintf(stderr, "Error: %s\n", i2clcd_strerror(err));
        ret = 1;
    }

cleanup:
    i2clcd_deinit(lcd);
    return ret;
}

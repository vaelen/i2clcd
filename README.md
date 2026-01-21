# libi2clcd

A C library for controlling HD44780 LCD displays via PCF8574 I2C backpack on Linux.

## Features

- Support for 16x2 (1602A) and 20x4 (2004A) LCD displays
- PCF8574/PCF8574A I2C backpack support
- Functions for text display, cursor control, and backlight
- Custom character support (CGRAM)
- Command-line utility (`lcdctl`) for scripting

## Building

```bash
make            # Build library and lcdctl
make examples   # Build example programs
make DEBUG=1    # Build with debug symbols
```

## Installation

```bash
sudo make install           # Install to /usr/local
sudo make PREFIX=/usr install  # Install to /usr
```

## Usage

### Command-Line Tool

```bash
# Initialize LCD
lcdctl init

# Display text
lcdctl line 0 "Hello, World!"
lcdctl line 1 "Line 2"

# Control backlight
lcdctl backlight off
lcdctl backlight on

# Clear display
lcdctl clear
lcdctl clear-line 0

# Cursor control
lcdctl cursor 5 0
lcdctl write "Text at cursor"
lcdctl home

# Options
lcdctl -d /dev/i2c-2 -a 0x3F -s 20x4 line 0 "Custom config"
```

### Library API

```c
#include <i2clcd.h>

int main(void)
{
    i2clcd_config_t config = I2CLCD_CONFIG_DEFAULT;
    i2clcd_t *lcd;

    /* Initialize */
    if (i2clcd_init(&config, &lcd) != I2CLCD_OK) {
        return 1;
    }

    /* Display text */
    i2clcd_set_line(lcd, 0, "Hello!");
    i2clcd_set_line(lcd, 1, "World!");

    /* Formatted output */
    i2clcd_set_cursor(lcd, 0, 1);
    i2clcd_printf(lcd, "Count: %d", 42);

    /* Cleanup */
    i2clcd_deinit(lcd);
    return 0;
}
```

Compile with:
```bash
gcc -o myapp myapp.c -li2clcd
```

## Configuration

The default configuration can be overridden:

| Option      | Default       | Description                         |
|-------------|---------------|-------------------------------------|
| i2c_device  | /dev/i2c-1    | I2C bus device                      |
| i2c_addr    | 0x27          | PCF8574 address (0x20-0x27, 0x38-0x3F) |
| size        | I2CLCD_16X2   | LCD size (I2CLCD_16X2, I2CLCD_20X4) |
| backlight   | true          | Initial backlight state             |

## Hardware Setup

Connect the PCF8574 I2C backpack to your Linux board's I2C bus:

| PCF8574 | Connection |
|---------|------------|
| VCC     | 5V         |
| GND     | GND        |
| SDA     | I2C SDA    |
| SCL     | I2C SCL    |

Detect the I2C address:
```bash
sudo i2cdetect -y 1
```

## Permissions

To run without sudo, add your user to the i2c group:
```bash
sudo usermod -aG i2c $USER
# Log out and back in
```

## License

MIT License - Copyright (c) 2026 Andrew C. Young

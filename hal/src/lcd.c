
#define _POSIX_C_SOURCE 200809L

#include "hal/lcd.h"

#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#define LCD_I2C_DEVICE "/dev/i2c-1"
#define LCD_I2C_ADDR   0x27

/* Common PCF8574 I2C backpack pin mapping */
#define LCD_RS         0x01
#define LCD_EN         0x04
#define LCD_BACKLIGHT  0x08

static int lcd_fd = -1;

/* Delay for the requested number of microseconds. */
static void delay_us(long microseconds)
{
    struct timespec delay = {
        .tv_sec = microseconds / 1000000,
        .tv_nsec = (microseconds % 1000000) * 1000
    };

    while (nanosleep(&delay, &delay) == -1 && errno == EINTR) {
        /* Resume the remaining delay if interrupted. */
    }
}

/* Write one byte to the I2C backpack. */
static int expander_write(uint8_t value)
{
    value |= LCD_BACKLIGHT;

    if (write(lcd_fd, &value, 1) != 1) {
        perror("LCD: I2C write failed");
        return -1;
    }

    return 0;
}

/* Pulse the LCD enable signal to latch a nibble. */
static int pulse_enable(uint8_t value)
{
    if (expander_write(value | LCD_EN) < 0) {
        return -1;
    }

    delay_us(1);

    if (expander_write(value & (uint8_t)~LCD_EN) < 0) {
        return -1;
    }

    delay_us(50);
    return 0;
}

/* Send the upper four bits of a byte to the LCD. */
static int write_nibble(uint8_t nibble, int register_select)
{
    uint8_t value = (nibble & 0xF0) |
                    (register_select ? LCD_RS : 0);

    if (expander_write(value) < 0) {
        return -1;
    }

    return pulse_enable(value);
}

/* Send one command or character as two four-bit nibbles. */
static int lcd_send(uint8_t value, int register_select)
{
    if (write_nibble(value & 0xF0, register_select) < 0) {
        return -1;
    }

    if (write_nibble((value << 4) & 0xF0,
                     register_select) < 0) {
        return -1;
    }

    return 0;
}

/* Send an LCD instruction. */
static int lcd_command(uint8_t command)
{
    if (lcd_send(command, 0) < 0) {
        return -1;
    }

    /* Clear and home instructions need extra processing time. */
    if (command == 0x01 || command == 0x02) {
        delay_us(2000);
    }

    return 0;
}

/* Send one display character. */
static int lcd_data(uint8_t data)
{
    return lcd_send(data, 1);
}

void LCD_init(void)
{
    if (lcd_fd >= 0) {
        return;
    }

    lcd_fd = open(LCD_I2C_DEVICE, O_RDWR);

    if (lcd_fd < 0) {
        perror("LCD: cannot open I2C device");
        return;
    }

    if (ioctl(lcd_fd, I2C_SLAVE, LCD_I2C_ADDR) < 0) {
        perror("LCD: cannot select I2C address");
        close(lcd_fd);
        lcd_fd = -1;
        return;
    }

    /* Wait for the LCD controller to power up. */
    delay_us(50000);

    /* Initialize an HD44780-compatible controller in 4-bit mode. */
    if (write_nibble(0x30, 0) < 0) {
        goto error;
    }
    delay_us(5000);

    if (write_nibble(0x30, 0) < 0) {
        goto error;
    }
    delay_us(200);

    if (write_nibble(0x30, 0) < 0) {
        goto error;
    }
    delay_us(200);

    if (write_nibble(0x20, 0) < 0) {
        goto error;
    }

    if (lcd_command(0x28) < 0) {
        goto error;
    } /* 4-bit mode, two-line layout */

    if (lcd_command(0x08) < 0) {
        goto error;
    } /* Display off */

    if (lcd_command(0x01) < 0) {
        goto error;
    } /* Clear display */

    if (lcd_command(0x06) < 0) {
        goto error;
    } /* Cursor increments */

    if (lcd_command(0x0C) < 0) {
        goto error;
    } /* Display on, cursor off */

    return;

error:
    fprintf(stderr, "LCD: initialization failed\n");
    close(lcd_fd);
    lcd_fd = -1;
}

void LCD_clear(void)
{
    if (lcd_fd < 0) {
        return;
    }

    lcd_command(0x01);
}

void LCD_setCursor(uint8_t row, uint8_t col)
{
    if (lcd_fd < 0) {
        return;
    }

    /* Standard DDRAM row offsets for common 16x2 and 20x4 LCDs. */
    static const uint8_t row_offsets[] = {
        0x00, 0x40, 0x14, 0x54
    };

    if (row >= sizeof(row_offsets) / sizeof(row_offsets[0])) {
        fprintf(stderr, "LCD: invalid row %u\n", (unsigned int)row);
        return;
    }

    /* Standard LCD controllers support up to 40 DDRAM columns per row. */
    if (col >= 40) {
        fprintf(stderr, "LCD: invalid column %u\n", (unsigned int)col);
        return;
    }

    lcd_command((uint8_t)(0x80 | (row_offsets[row] + col)));
}

void LCD_printString(const char *str)
{
    if (lcd_fd < 0 || str == NULL) {
        return;
    }

    while (*str != '\0') {
        if (lcd_data((uint8_t)*str) < 0) {
            return;
        }

        str++;
    }
}

void LCD_cleanup(void)
{
    if (lcd_fd < 0) {
        return;
    }

    LCD_clear();

    close(lcd_fd);
    lcd_fd = -1;
}

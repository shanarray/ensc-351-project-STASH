
#define _POSIX_C_SOURCE 200809L

#include "keypad.h"
#include "hal/lcd.h"

#include <gpiod.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

#define ROWS 4
#define COLS 4
#define MAX_MINUTES 999
#define MAX_DIGITS 3

typedef struct {
    struct gpiod_chip *chip;
    struct gpiod_line_request *request;
    unsigned int offset;
} GpioLine;

static const char keys[ROWS][COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

static const char *row_chips[ROWS] = {
    "/dev/gpiochip1", "/dev/gpiochip2",
    "/dev/gpiochip0", "/dev/gpiochip0"
};

static const unsigned int row_offsets[ROWS] = {38, 8, 7, 10};

static const char *col_chips[COLS] = {
    "/dev/gpiochip2", "/dev/gpiochip2",
    "/dev/gpiochip2", "/dev/gpiochip1"
};

static const unsigned int col_offsets[COLS] = {15, 17, 7, 36};

static void delay_ms(long ms)
{
    struct timespec t = {
        .tv_sec = ms / 1000,
        .tv_nsec = (ms % 1000) * 1000000L
    };

    nanosleep(&t, NULL);
}

static GpioLine rows[ROWS];
static GpioLine cols[COLS];

static int init_gpio_line(
    GpioLine *line,
    const char *chip_path,
    unsigned int offset,
    bool output)
{
    line->chip = NULL;
    line->request = NULL;
    line->offset = offset;

    line->chip = gpiod_chip_open(chip_path);
    if (!line->chip)
        goto error;

    struct gpiod_line_settings *settings =
        gpiod_line_settings_new();
    struct gpiod_line_config *config =
        gpiod_line_config_new();
    struct gpiod_request_config *request_config =
        gpiod_request_config_new();

    if (!settings || !config || !request_config) {
        if (settings) gpiod_line_settings_free(settings);
        if (config) gpiod_line_config_free(config);
        if (request_config)
            gpiod_request_config_free(request_config);
        goto error;
    }

    int result;

    if (output) {
        result = gpiod_line_settings_set_direction(
            settings, GPIOD_LINE_DIRECTION_OUTPUT);

        if (result == 0)
            result = gpiod_line_settings_set_output_value(
                settings, GPIOD_LINE_VALUE_ACTIVE);
    } else {
        result = gpiod_line_settings_set_direction(
            settings, GPIOD_LINE_DIRECTION_INPUT);

        if (result == 0)
            result = gpiod_line_settings_set_bias(
                settings, GPIOD_LINE_BIAS_PULL_UP);
    }

    if (result < 0)
        goto config_error;

    if (gpiod_line_config_add_line_settings(
            config, &offset, 1, settings) < 0)
        goto config_error;

    gpiod_request_config_set_consumer(
        request_config, "study_menu_keypad");

    line->request = gpiod_chip_request_lines(
        line->chip, request_config, config);

    gpiod_line_settings_free(settings);
    gpiod_line_config_free(config);
    gpiod_request_config_free(request_config);

    if (!line->request)
        goto error;

    return 0;

config_error:
    gpiod_line_settings_free(settings);
    gpiod_line_config_free(config);
    gpiod_request_config_free(request_config);

error:
    perror("Keypad GPIO initialization failed");

    if (line->request) {
        gpiod_line_request_release(line->request);
        line->request = NULL;
    }

    if (line->chip) {
        gpiod_chip_close(line->chip);
        line->chip = NULL;
    }

    return -1;
}

static void cleanup_gpio_line(GpioLine *line)
{
    if (line->request) {
        gpiod_line_request_release(line->request);
        line->request = NULL;
    }

    if (line->chip) {
        gpiod_chip_close(line->chip);
        line->chip = NULL;
    }
}

int keypad_init(void)
{
    for (int i = 0; i < ROWS; i++) {
        if (init_gpio_line(
                &rows[i], row_chips[i], row_offsets[i], true) < 0)
            goto error;
    }

    for (int i = 0; i < COLS; i++) {
        if (init_gpio_line(
                &cols[i], col_chips[i], col_offsets[i], false) < 0)
            goto error;
    }

    return 0;

error:
    keypad_cleanup();
    return -1;
}

void keypad_cleanup(void)
{
    for (int i = 0; i < ROWS; i++) {
        if (rows[i].request)
            gpiod_line_request_set_value(
                rows[i].request, rows[i].offset,
                GPIOD_LINE_VALUE_ACTIVE);

        cleanup_gpio_line(&rows[i]);
    }

    for (int i = 0; i < COLS; i++)
        cleanup_gpio_line(&cols[i]);
}

static int set_row(int r, enum gpiod_line_value value)
{
    return gpiod_line_request_set_value(
        rows[r].request, rows[r].offset, value);
}

char keypad_scan(void)
{
    for (int r = 0; r < ROWS; r++)
        if (set_row(r, GPIOD_LINE_VALUE_ACTIVE) < 0)
            return '\0';

    for (int r = 0; r < ROWS; r++) {
        if (set_row(r, GPIOD_LINE_VALUE_INACTIVE) < 0)
            return '\0';

        delay_ms(1);

        for (int c = 0; c < COLS; c++) {
            enum gpiod_line_value value =
                gpiod_line_request_get_value(
                    cols[c].request, cols[c].offset);

            if (value == GPIOD_LINE_VALUE_INACTIVE) {
                set_row(r, GPIOD_LINE_VALUE_ACTIVE);
                return keys[r][c];
            }
        }

        set_row(r, GPIOD_LINE_VALUE_ACTIVE);
    }

    return '\0';
}

char keypad_get_key(void)
{
    while (1) {
        char key = keypad_scan();

        if (key != '\0') {
            delay_ms(25);

            if (keypad_scan() == key) {
                while (keypad_scan() != '\0')
                    delay_ms(10);

                return key;
            }
        }

        delay_ms(10);
    }
}

static void display_custom_entry(
    const char *prompt,
    const char *digits)
{
    LCD_clear();

    LCD_setCursor(0, 0);
    LCD_printString(prompt);

    LCD_setCursor(1, 0);
    LCD_printString(digits[0] ? digits : "_");
}

int keypad_enter_minutes(const char *prompt)
{
    char digits[MAX_DIGITS + 1] = "";
    int length = 0;

    display_custom_entry(prompt, digits);

    while (1) {
        char key = keypad_get_key();

        if (key >= '0' && key <= '9') {
            if (length < MAX_DIGITS) {
                digits[length++] = key;
                digits[length] = '\0';
                display_custom_entry(prompt, digits);
            }
        }
        else if (key == '*') {
            if (length == 0)
                return -1;

            length = 0;
            digits[0] = '\0';
            display_custom_entry(prompt, digits);
        }
        else if (key == '#') {
            if (length == 0) {
                LCD_clear();
                LCD_setCursor(0, 0);
                LCD_printString("Enter a number");
                delay_ms(800);
                display_custom_entry(prompt, digits);
                continue;
            }

            int minutes = atoi(digits);

            if (minutes < 1 || minutes > MAX_MINUTES) {
                LCD_clear();
                LCD_setCursor(0, 0);
                LCD_printString("Range: 1-999");
                delay_ms(800);
                display_custom_entry(prompt, digits);
                continue;
            }

            return minutes;
        }
    }

    return -1;
}

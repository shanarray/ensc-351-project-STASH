#include "hal/led.h"
#include <gpiod.h>
#include <stdio.h>
#include <stdlib.h>

static struct gpiod_chip *chips[3];
static struct gpiod_line *lines[3];

static void open_output(LedColor color, const char *chip_name, unsigned int offset)
{
    chips[color] = gpiod_chip_open_by_name(chip_name);
    if (!chips[color]) {
        perror("gpiod_chip_open_by_name");
        exit(EXIT_FAILURE);
    }
    lines[color] = gpiod_chip_get_line(chips[color], offset);
    if (!lines[color] || gpiod_line_request_output(lines[color], "led-feedback-demo", 0) < 0) {
        perror("gpiod_line_request_output");
        exit(EXIT_FAILURE);
    }
}

void Led_init(void)
{
    open_output(LED_GREEN, "gpiochip0", 10);
    open_output(LED_RED, "gpiochip1", 42);
    open_output(LED_YELLOW, "gpiochip2", 15);
}

void Led_setState(LedColor color, bool state)
{
    if (color < LED_RED || color > LED_GREEN || !lines[color]) return;
    if (gpiod_line_set_value(lines[color], state ? 1 : 0) < 0) {
        perror("gpiod_line_set_value");
        exit(EXIT_FAILURE);
    }
}

bool Led_getState(LedColor color)
{
    if (color < LED_RED || color > LED_GREEN || !lines[color]) return false;
    return gpiod_line_get_value(lines[color]) == 1;
}

void Led_cleanup(void)
{
    for (int i = 0; i < 3; i++) {
        if (lines[i]) {
            gpiod_line_set_value(lines[i], 0);
            gpiod_line_release(lines[i]);
            lines[i] = NULL;
        }
        if (chips[i]) {
            gpiod_chip_close(chips[i]);
            chips[i] = NULL;
        }
    }
}

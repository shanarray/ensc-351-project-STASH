/* LED GPIO output for libgpiod 2.x (shared library libgpiod.so.3). */
#include "hal/led.h"

#include <gpiod.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LED_COUNT 3
#define MAX_CHIPS 10

static struct gpiod_line_request *requests[LED_COUNT];
static unsigned int offsets[LED_COUNT];
static bool values[LED_COUNT];

/* Set these environment variables to three unreserved GPIO line names. */
static const char *const env_names[LED_COUNT] = {
    [LED_RED] = "STASH_LED_RED_GPIO",
    [LED_YELLOW] = "STASH_LED_YELLOW_GPIO",
    [LED_GREEN] = "STASH_LED_GREEN_GPIO"
};

static struct gpiod_line_request *request_output(struct gpiod_chip *chip,
                                                 unsigned int offset)
{
    struct gpiod_line_settings *settings = gpiod_line_settings_new();
    struct gpiod_line_config *config = gpiod_line_config_new();
    struct gpiod_request_config *request_config = gpiod_request_config_new();
    struct gpiod_line_request *request = NULL;

    if (settings && config && request_config) {
        gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);
        gpiod_line_settings_set_output_value(settings, GPIOD_LINE_VALUE_INACTIVE);
        gpiod_request_config_set_consumer(request_config, "stash-led-feedback");
        if (gpiod_line_config_add_line_settings(config, &offset, 1, settings) == 0)
            request = gpiod_chip_request_lines(chip, request_config, config);
    }

    if (request_config) gpiod_request_config_free(request_config);
    if (config) gpiod_line_config_free(config);
    if (settings) gpiod_line_settings_free(settings);
    return request;
}

static int open_named_output(LedColor color, const char *line_name)
{
    char chip_path[40];
    for (int i = 0; i < MAX_CHIPS; ++i) {
        snprintf(chip_path, sizeof(chip_path), "/dev/gpiochip%d", i);
        struct gpiod_chip *chip = gpiod_chip_open(chip_path);
        if (!chip) continue;

        int offset = gpiod_chip_get_line_offset_from_name(chip, line_name);
        if (offset >= 0) {
            struct gpiod_line_request *req = request_output(chip, (unsigned int)offset);
            gpiod_chip_close(chip);
            if (!req) {
                fprintf(stderr, "LED: cannot request %s (busy or permission denied)\n", line_name);
                return -1;
            }
            requests[color] = req;
            offsets[color] = (unsigned int)offset;
            printf("LED: %s -> %s offset %d\n", line_name, chip_path, offset);
            return 0;
        }
        gpiod_chip_close(chip);
    }
    fprintf(stderr, "LED: GPIO line %s not found\n", line_name);
    return -1;
}

void Led_cleanup(void)
{
    for (int i = 0; i < LED_COUNT; ++i) {
        if (requests[i]) {
            gpiod_line_request_set_value(requests[i], offsets[i], GPIOD_LINE_VALUE_INACTIVE);
            gpiod_line_request_release(requests[i]);
            requests[i] = NULL;
        }
        values[i] = false;
    }
}

void Led_init(void)
{
    const char *names[LED_COUNT];
    Led_cleanup();
    for (int i = 0; i < LED_COUNT; ++i) {
        names[i] = getenv(env_names[i]);
        if (!names[i] || !names[i][0]) {
            fprintf(stderr, "LED: set %s to an unused GPIO line name\n", env_names[i]);
            return;
        }
        for (int j = 0; j < i; ++j) {
            if (strcmp(names[i], names[j]) == 0) {
                fprintf(stderr, "LED: duplicate GPIO line name %s\n", names[i]);
                return;
            }
        }
    }
    for (int i = 0; i < LED_COUNT; ++i) {
        if (open_named_output((LedColor)i, names[i]) != 0) {
            Led_cleanup();
            return;
        }
    }
}

void Led_setState(LedColor color, bool on)
{
    if (color < LED_RED || color > LED_GREEN || !requests[color]) return;
    if (gpiod_line_request_set_value(requests[color], offsets[color],
            on ? GPIOD_LINE_VALUE_ACTIVE : GPIOD_LINE_VALUE_INACTIVE) != 0) {
        perror("LED: set value");
        return;
    }
    values[color] = on;
}

bool Led_getState(LedColor color)
{
    if (color < LED_RED || color > LED_GREEN) return false;
    return values[color];
}

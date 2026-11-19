#define _POSIX_C_SOURCE 200809L
#include "buzzer_gpio.h"
#include <gpiod.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

static struct gpiod_line_request *request = NULL;
static unsigned int offset = 0;
static bool last_on = false;
static bool timer_requested = false;
static bool motion_active = false;
static uint64_t motion_started_ms = 0;

static uint64_t buzzer_now_ms(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (uint64_t)t.tv_sec * 1000u + (uint64_t)t.tv_nsec / 1000000u;
}

/* One output, with motion-alert priority over timer completion. */
static void buzzer_output(bool on)
{
    if (!request || on == last_on) return;
    if (gpiod_line_request_set_value(request, offset,
            on ? GPIOD_LINE_VALUE_ACTIVE : GPIOD_LINE_VALUE_INACTIVE) != 0) {
        perror("Buzzer: failed to set GPIO13");
        return;
    }
    last_on = on;
}

void Buzzer_update(void)
{
    bool output = timer_requested;
    if (motion_active) {
        uint64_t elapsed = buzzer_now_ms() - motion_started_ms;
        if (elapsed >= 950u) {
            motion_active = false;
        } else {
            uint64_t slot = elapsed % 350u;
            output = (slot < 250u);
        }
    }
    buzzer_output(output);
}

void Buzzer_motion_alert_start(void)
{
    if (!request) return;
    motion_started_ms = buzzer_now_ms();
    motion_active = true;
    Buzzer_update();
}


int Buzzer_init(void)
{
    if (request) return 0;

    for (int i = 0; i < 10; ++i) {
        char path[32];
        snprintf(path, sizeof(path), "/dev/gpiochip%d", i);
        struct gpiod_chip *chip = gpiod_chip_open(path);
        if (!chip) continue;

        int line = gpiod_chip_get_line_offset_from_name(chip, "GPIO13");
        if (line < 0) {
            gpiod_chip_close(chip);
            continue;
        }

        unsigned int candidate_offset = (unsigned int)line;
        struct gpiod_line_settings *settings = gpiod_line_settings_new();
        struct gpiod_line_config *config = gpiod_line_config_new();
        struct gpiod_request_config *rc = gpiod_request_config_new();
        struct gpiod_line_request *candidate = NULL;

        if (settings && config && rc &&
            gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT) == 0 &&
            gpiod_line_settings_set_output_value(settings, GPIOD_LINE_VALUE_INACTIVE) == 0) {
            gpiod_request_config_set_consumer(rc, "stash-buzzer");
            if (gpiod_line_config_add_line_settings(config, &candidate_offset, 1, settings) == 0)
                candidate = gpiod_chip_request_lines(chip, rc, config);
        }

        if (rc) gpiod_request_config_free(rc);
        if (config) gpiod_line_config_free(config);
        if (settings) gpiod_line_settings_free(settings);
        gpiod_chip_close(chip);

        if (!candidate) {
            fprintf(stderr, "Buzzer: cannot request GPIO13 on %s\n", path);
            return -1;
        }

        request = candidate;
        offset = candidate_offset;
        last_on = false;
        timer_requested = false;
        motion_active = false;
        return 0;
    }
    return -1;
}

void Buzzer_write(bool on)
{
    timer_requested = on;
    Buzzer_update();
}

void Buzzer_cleanup(void)
{
    if (!request) return;
    motion_active = false;
    timer_requested = false;
    buzzer_output(false);
    gpiod_line_request_release(request);
    request = NULL;
    last_on = false;
}
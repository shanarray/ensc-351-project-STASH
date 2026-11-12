#include "buzzer_gpio.h"
#include <gpiod.h>
#include <stdio.h>

static struct gpiod_line_request *request = NULL;
static unsigned int offset = 0;
static bool last_on = false;

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
        return 0;
    }
    return -1;
}

void Buzzer_write(bool on)
{
    if (!request) {
        fprintf(stderr, "Buzzer: output was not initialized\n");
        return;
    }
    if (on == last_on) return;

    if (gpiod_line_request_set_value(request, offset,
            on ? GPIOD_LINE_VALUE_ACTIVE : GPIOD_LINE_VALUE_INACTIVE) != 0) {
        perror("Buzzer: failed to set GPIO13");
        return;
    }
    last_on = on;
    int value = gpiod_line_request_get_value(request, offset);
    printf("Buzzer GPIO13 requested %s; readback=%d\n", on ? "ON" : "OFF", value);
    fflush(stdout);
    if (value < 0)
        fprintf(stderr, "Buzzer: output readback failed\n");
}

void Buzzer_cleanup(void)
{
    if (!request) return;
    Buzzer_write(false);
    gpiod_line_request_release(request);
    request = NULL;
    last_on = false;
}
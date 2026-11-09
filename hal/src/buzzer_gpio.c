#include "hal/buzzer_gpio.h"
#include <gpiod.h>
#include <stdio.h>
#include <time.h>

static struct gpiod_line_request *request;
static unsigned int offset;

int Buzzer_init(void)
{
    for (int i = 0; i < 10; i++) {
        char path[32];
        snprintf(path, sizeof(path), "/dev/gpiochip%d", i);
        struct gpiod_chip *chip = gpiod_chip_open(path);
        if (!chip) continue;
        int line = gpiod_chip_get_line_offset_from_name(chip, "GPIO13");
        if (line >= 0) {
            offset = (unsigned int)line;
            struct gpiod_line_settings *settings = gpiod_line_settings_new();
            struct gpiod_line_config *config = gpiod_line_config_new();
            struct gpiod_request_config *rc = gpiod_request_config_new();
            if (settings && config && rc) {
                gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);
                gpiod_line_settings_set_output_value(settings, GPIOD_LINE_VALUE_INACTIVE);
                gpiod_request_config_set_consumer(rc, "stash-buzzer");
                if (gpiod_line_config_add_line_settings(config, &offset, 1, settings) == 0)
                    request = gpiod_chip_request_lines(chip, rc, config);
            }
            if (rc) gpiod_request_config_free(rc);
            if (config) gpiod_line_config_free(config);
            if (settings) gpiod_line_settings_free(settings);
            gpiod_chip_close(chip);
            if (request) {
                puts("Buzzer ready: GPIO13 (physical pin 33)");
                return 0;
            }
            fputs("GPIO13 is unavailable (in use, permissions, or pin mux).\n", stderr);
            return -1;
        }
        gpiod_chip_close(chip);
    }
    fputs("GPIO13 not found; check gpioinfo and pin mux.\n", stderr);
    return -1;
}

void Buzzer_write(bool on)
{
   /* if (request && gpiod_line_request_set_value(request, offset,
        on ? GPIOD_LINE_VALUE_ACTIVE : GPIOD_LINE_VALUE_INACTIVE) != 0)
        perror("Buzzer GPIO");
    */
   // PNP transistor
   // LOW = ON and HIGH = OFF
   enum gpiod_line_value value = on ? GPIOD_LINE_VALUE_INACTIVE : GPIOD_LINE_VALUE_ACTIVE;

   if (gpiod_line_request_set_value(request, offset, value) != 0) {
        perror("Buzzer GPIO");
   }
}

// Generate a tone for a passive buzzer
void Buzzer_beep(int frequency, int duration_ms)
{
    if (!request || frequency <= 0 || duration_ms <= 0) 
        return;
    long half_period_ns = 500000000L / frequency;

    struct timespec delay = {
        .tv_sec = 0,
        .tv_nsec = half_period_ns
    };

    int cycles = (frequency * duration_ms) / 1000;

    for (int i = 0; i < cycles; i++) {
        // PNP ON, GPIO LOW
        Buzzer_write(true);

        nanosleep(&delay, NULL);

        // PNP OFF, GPIO HIGH
        Buzzer_write(false);

        nanosleep(&delay, NULL);
    }
    // Buzzer off
    Buzzer_write(false);
}

void Buzzer_cleanup(void)
{
    if (request) {
        Buzzer_write(false);
        gpiod_line_request_release(request);
        request = NULL;
    }
}
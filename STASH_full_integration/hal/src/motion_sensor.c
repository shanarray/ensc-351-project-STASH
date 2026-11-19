#include "hal/motion_sensor.h"
#include <gpiod.h>
#include <stdio.h>

#define MOTION_GPIO "GPIO20"
static struct gpiod_chip *chip;
static struct gpiod_line_request *request;
static unsigned int offset;

int MotionSensor_init(void)
{
    if (request) return 0;
    for (int i = 0; i < 10; ++i) {
        char path[32];
        snprintf(path, sizeof(path), "/dev/gpiochip%d", i);
        struct gpiod_chip *candidate_chip = gpiod_chip_open(path);
        if (!candidate_chip) continue;
        int line = gpiod_chip_get_line_offset_from_name(candidate_chip, MOTION_GPIO);
        if (line < 0) { gpiod_chip_close(candidate_chip); continue; }
        unsigned int candidate_offset = (unsigned int)line;
        struct gpiod_line_settings *settings = gpiod_line_settings_new();
        struct gpiod_line_config *config = gpiod_line_config_new();
        struct gpiod_request_config *req = gpiod_request_config_new();
        if (settings && config && req &&
            gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT) == 0 &&
            gpiod_line_config_add_line_settings(config, &candidate_offset, 1, settings) == 0) {
            gpiod_request_config_set_consumer(req, "stash-motion");
            request = gpiod_chip_request_lines(candidate_chip, req, config);
        }
        if (req) gpiod_request_config_free(req);
        if (config) gpiod_line_config_free(config);
        if (settings) gpiod_line_settings_free(settings);
        if (!request) {
            gpiod_chip_close(candidate_chip);
            fprintf(stderr, "MotionSensor: cannot request %s on %s\n", MOTION_GPIO, path);
            return -1;
        }
        chip = candidate_chip;
        offset = candidate_offset;
        printf("Motion sensor initialized: %s\n", MOTION_GPIO);
        return 0;
    }
    fprintf(stderr, "MotionSensor: %s not found\n", MOTION_GPIO);
    return -1;
}

bool MotionSensor_detected(void)
{
    if (!request) return false;
    enum gpiod_line_value value = gpiod_line_request_get_value(request, offset);
    if (value == GPIOD_LINE_VALUE_ERROR) {
        perror("MotionSensor: failed to read GPIO");
        return false;
    }
    return value == GPIOD_LINE_VALUE_ACTIVE;
}

void MotionSensor_cleanup(void)
{
    if (request) gpiod_line_request_release(request);
    if (chip) gpiod_chip_close(chip);
    request = NULL;
    chip = NULL;
}

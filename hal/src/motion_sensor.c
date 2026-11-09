#include "hal/motion_sensor.h"

#include <gpiod.h>
#include <stdio.h>

// Motion sensor GPIO
#define MOTION_GPIO "GPIO20"

static struct gpiod_chip *chip = NULL;
static struct gpiod_line_request *request = NULL;
static unsigned int offset = 0;

// Initialize motion sensor
int MotionSensor_init(void)
{
    for (int i = 0; i < 10; i++) {

        char path [32];
        snprintf(path, sizeof(path), "/dev/gpiochip%d", i);
        chip = gpiod_chip_open(path);

        if (!chip)
            continue;

        // Find GPIO20
        int line_offset = gpiod_chip_get_line_offset_from_name(chip, MOTION_GPIO);

        if (line_offset >= 0) {

            offset = (unsigned int)line_offset;

            // Line settings
            struct gpiod_line_settings *settings = gpiod_line_settings_new();
            struct gpiod_line_config *config = gpiod_line_config_new();
            struct gpiod_request_config *req = gpiod_request_config_new();

            if (settings == NULL || config == NULL || req == NULL) {

                perror("MotionSensor: failed to create GPIO configuration");

                if (settings)
                    gpiod_line_settings_free(settings);
                
                if (config) 
                    gpiod_line_config_free(config);

                if (req)
                    gpiod_request_config_free(req);

                gpiod_chip_close(chip);
                chip = NULL;

                return -1; 
            }

            // GPIO input configuration
            gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);

            // Consumer name
            gpiod_request_config_set_consumer(req, "motion_sensor");

            // GPIO line settings
            if (gpiod_line_config_add_line_settings(config, &offset, 1, settings) != 0) {

                perror("MotionSensor: failed to configure GPIO");

                gpiod_request_config_free(req);
                gpiod_line_config_free(config);
                gpiod_line_settings_free(settings);

                gpiod_chip_close(chip);
                chip = NULL;

                return -1;
            }

            // Request GPIO line
            request = gpiod_chip_request_lines(chip, req, config);

            // Release temp configuration resources
            gpiod_request_config_free(req);
            gpiod_line_config_free(config);
            gpiod_line_settings_free(settings);

            if (request != NULL) {
                printf("Motion sensor initialized: %s\n", MOTION_GPIO);
                return 0;
            }

            perror("MotionSensor: failed to request GPIO line");
            
            gpiod_chip_close(chip);
            chip = NULL;
            
            return -1;
        }

        // GPIO not found
        gpiod_chip_close(chip);
        chip = NULL;
    }

    fprintf(stderr, "MotionSensor: GPIO20 not found\n");
    return -1;
}

// Check for motion detection
bool MotionSensor_detected(void)
{
    if (request == NULL)
        return false;

    // Read GPIO value
    enum gpiod_line_value value = gpiod_line_request_get_value(request, offset);

    if (value == GPIOD_LINE_VALUE_ERROR) {
        perror("MotionSensor: faile to read GPIO");
        return false;
    }

    printf("GPIO value: %d\n", value);

    // Motion detected
    return value == GPIOD_LINE_VALUE_ACTIVE;
}

// Cleanup motion sensor
void MotionSensor_cleanup(void)
{
    if (request != NULL) {
        gpiod_line_request_release(request);
        request = NULL;
    }

    if (chip != NULL) {
        gpiod_chip_close(chip);
        chip = NULL;
    }
}
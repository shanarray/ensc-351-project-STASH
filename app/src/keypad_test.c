
#define _POSIX_C_SOURCE 200809L

#include <gpiod.h>
#include <stdio.h>
#include <stdbool.h>
#include <signal.h>
#include <time.h>

#define ROWS 4
#define COLS 4

static volatile sig_atomic_t running = 1;

static const char keys[ROWS][COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

typedef struct {
    struct gpiod_chip *chip;
    struct gpiod_line_request *request;
    unsigned int offset;
} GpioLine;

/* Mapping from the proposed wiring to Linux GPIO chip offsets. */
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

static GpioLine rows[ROWS];
static GpioLine cols[COLS];

static void handle_signal(int sig)
{
    (void)sig;
    running = 0;
}

static void delay_ms(long ms)
{
    struct timespec t = {
        .tv_sec = ms / 1000,
        .tv_nsec = (ms % 1000) * 1000000L
    };
    while (nanosleep(&t, &t) == -1 && running) {}
}

static int init_line(GpioLine *line, const char *chip_path,
                     unsigned int offset, bool output)
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
        if (request_config) gpiod_request_config_free(request_config);
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

    gpiod_request_config_set_consumer(request_config, "keypad_test");

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
    perror("Could not initialize GPIO line");
    if (line->chip) {
        gpiod_chip_close(line->chip);
        line->chip = NULL;
    }
    return -1;
}

static void cleanup_line(GpioLine *line)
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

static int set_row(int row, enum gpiod_line_value value)
{
    return gpiod_line_request_set_value(
        rows[row].request, rows[row].offset, value);
}

/* Returns a key character, or '\0' when no key is detected. */
static char scan_keypad(void)
{
    for (int r = 0; r < ROWS; r++) {
        if (set_row(r, GPIOD_LINE_VALUE_ACTIVE) < 0)
            return '\0';
    }

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

int main(void)
{
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    printf("Initializing 4x4 keypad test...\n");

    int rows_ready = 0;
    int cols_ready = 0;

    for (int i = 0; i < ROWS; i++) {
        if (init_line(&rows[i], row_chips[i],
                      row_offsets[i], true) < 0)
            goto cleanup;
        rows_ready++;
    }

    for (int i = 0; i < COLS; i++) {
        if (init_line(&cols[i], col_chips[i],
                      col_offsets[i], false) < 0)
            goto cleanup;
        cols_ready++;
    }

    printf("Keypad ready. Press keys; Ctrl+C exits.\n");

    while (running) {
        char key = scan_keypad();

        if (key != '\0') {
            delay_ms(25);

            if (scan_keypad() == key) {
                printf("Key pressed: %c\n", key);
                fflush(stdout);

                while (running && scan_keypad() != '\0')
                    delay_ms(10);
            }
        }

        delay_ms(10);
    }

cleanup:
    for (int i = 0; i < cols_ready; i++)
        cleanup_line(&cols[i]);

    for (int i = 0; i < rows_ready; i++)
        cleanup_line(&rows[i]);

    printf("\nKeypad test finished.\n");
    return (rows_ready == ROWS && cols_ready == COLS) ? 0 : 1;
}

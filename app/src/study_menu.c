#define _POSIX_C_SOURCE 200809L

#include "hal/lcd.h"
#include "hal/joystick.h"
#include "hal/timer.h"
#include "keypad.h"
#include "session.h"

#include <gpiod.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define MENU_COUNT 7
#define BUTTON_CHIP "/dev/gpiochip1"
#define BUTTON_LINE 33

typedef struct {
    int study_minutes;
    int break_minutes;
    int pomodoro;
} StudyMode;

static const char *menu_options[MENU_COUNT] = {
    "Pomodoro 25/5",
    "Study 60 min",
    "Study 90 min",
    "Study 120 min",
    "Study 30 min",
    "Study 180 min",
    "Custom Session"
};

static const StudyMode modes[MENU_COUNT] = {
    {25, 5, 1},
    {60, 10, 0},
    {90, 20, 0},
    {120, 30, 0},
    {30, 5, 0},
    {180, 30, 0},
    {0, 0, 0}
};


static volatile sig_atomic_t running = 1;

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

/* ---------- Joystick SW button ---------- */

static struct gpiod_line_request *button_init(
    struct gpiod_chip **chip)
{
    struct gpiod_line_settings *settings =
        gpiod_line_settings_new();
    struct gpiod_line_config *config =
        gpiod_line_config_new();
    struct gpiod_request_config *request_config =
        gpiod_request_config_new();

    if (!settings || !config || !request_config)
        goto error;

    if (gpiod_line_settings_set_direction(
            settings, GPIOD_LINE_DIRECTION_INPUT) < 0)
        goto error;

    if (gpiod_line_settings_set_bias(
            settings, GPIOD_LINE_BIAS_PULL_UP) < 0)
        goto error;

    unsigned int offset = BUTTON_LINE;

    if (gpiod_line_config_add_line_settings(
            config, &offset, 1, settings) < 0)
        goto error;

    gpiod_request_config_set_consumer(
        request_config, "study_menu");

    *chip = gpiod_chip_open(BUTTON_CHIP);
    if (!*chip)
        goto error;

    struct gpiod_line_request *request =
        gpiod_chip_request_lines(*chip, request_config, config);

    gpiod_line_settings_free(settings);
    gpiod_line_config_free(config);
    gpiod_request_config_free(request_config);

    return request;

error:
    if (settings) gpiod_line_settings_free(settings);
    if (config) gpiod_line_config_free(config);
    if (request_config) gpiod_request_config_free(request_config);

    if (*chip) {
        gpiod_chip_close(*chip);
        *chip = NULL;
    }

    return NULL;
}

static bool button_is_pressed(
    struct gpiod_line_request *request)
{
    enum gpiod_line_value value =
        gpiod_line_request_get_value(request, BUTTON_LINE);

    /* Active-low: raw low is reported as logical inactive. */
    return value == GPIOD_LINE_VALUE_INACTIVE;
}

 
/* ---------- LCD screens ---------- */

static void display_menu(int selected)
{
    int next = (selected + 1) % MENU_COUNT;

    LCD_clear();
    LCD_setCursor(0, 0);
    LCD_printString(">");
    LCD_printString(menu_options[selected]);

    LCD_setCursor(1, 0);
    LCD_printString(" ");
    LCD_printString(menu_options[next]);
}


/* ---------- Custom session setup ---------- */

static bool custom_session(
    struct gpiod_line_request *button,
    int *study,
    int *rest)
{
    (void)button;

    *study = keypad_enter_minutes("Study minutes:");

    if (*study < 0)
        return false;

    *rest = keypad_enter_minutes("Break minutes:");

    if (*rest < 0)
        return false;

    display_confirmation(*study, *rest);

    delay_ms(1500);

    return true;
}

/* ---------- Main menu ---------- */

int main(void)
{
    int selected = 0;
    bool previous_direction_active = false;

    struct gpiod_chip *button_chip = NULL;
    struct gpiod_line_request *button = NULL;

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    printf("Starting Study Station...\n");

    LCD_init();
    Joystick_init();

    button = button_init(&button_chip);

    if (!button) {
        fprintf(stderr, "Could not initialize joystick SW.\n");
        Joystick_cleanup();
        LCD_cleanup();
        return 1;
    }

    if (keypad_init() < 0) {
        fprintf(stderr, "Could not initialize keypad GPIOs.\n");
        gpiod_line_request_release(button);
        gpiod_chip_close(button_chip);
        Joystick_cleanup();
        LCD_cleanup();
        return 1;
    }

    printf("Navigate with joystick. Press SW to select.\n");
    printf("Custom Session uses keypad digits, # to confirm, * to clear/cancel.\n");

    display_menu(selected);

    while (running) {
        JoystickDirection direction = Joystick_getDirection();

        bool moving =
            direction == JOYSTICK_UP || direction == JOYSTICK_DOWN;

        if (moving && !previous_direction_active) {
            if (direction == JOYSTICK_UP)
                selected = (selected - 1 + MENU_COUNT) % MENU_COUNT;
            else
                selected = (selected + 1) % MENU_COUNT;

            display_menu(selected);
        }

        previous_direction_active = moving;

        if (button_is_pressed(button)) {
            delay_ms(30);

            if (button_is_pressed(button)) {
                while (running && button_is_pressed(button))
                    delay_ms(20);

                if (!running)
                    break;

                int study = 0;
                int rest = 0;
                bool is_pomodoro = false;
                bool have_session = false;

                if (selected == MENU_COUNT - 1) {
                    have_session = custom_session(button, &study, &rest);
                } else {
                    study = modes[selected].study_minutes;
                    rest = modes[selected].break_minutes;
                    is_pomodoro = modes[selected].pomodoro;
                    have_session = true;
                }

                if (have_session) {
    if (confirm_session(button, study, rest, &running)) {
        printf("Starting %s: study %d min, break %d min\n",
               menu_options[selected], study, rest);

        run_session(study, rest, is_pomodoro, &running);
    }

    display_menu(selected);
} else {
    display_menu(selected);
}
            }
        }

        delay_ms(20);
    }

 printf("\nCleaning up...\n");

    keypad_cleanup();
    gpiod_line_request_release(button);
    gpiod_chip_close(button_chip);
    Joystick_cleanup();
    LCD_cleanup();

    printf("Study Station finished.\n");
    return 0;
}

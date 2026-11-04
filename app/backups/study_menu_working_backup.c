#define _POSIX_C_SOURCE 200809L

#include "hal/lcd.h"
#include "hal/joystick.h"
#include "hal/timer.h"

#include <gpiod.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define MENU_COUNT 7
#define ROWS 4
#define COLS 4
#define BUTTON_CHIP "/dev/gpiochip1"
#define BUTTON_LINE 33
#define MAX_MINUTES 999
#define MAX_DIGITS 3

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

static const char keys[ROWS][COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

/* GPIO chip/offset mapping from the keypad wiring test. */
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

typedef struct {
    struct gpiod_chip *chip;
    struct gpiod_line_request *request;
    unsigned int offset;
} GpioLine;

static GpioLine rows[ROWS];
static GpioLine cols[COLS];

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

/* Call only after the initial button press has been released. */
static bool wait_for_button_press(
    struct gpiod_line_request *button)
{
    while (running && !button_is_pressed(button))
        delay_ms(20);

    if (!running)
        return false;

    delay_ms(30);

    if (!button_is_pressed(button))
        return false;

    while (running && button_is_pressed(button))
        delay_ms(20);

    return running;
}
 
/* ---------- Keypad ---------- */

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

static int keypad_init(void)
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
    for (int i = 0; i < ROWS; i++)
        cleanup_gpio_line(&rows[i]);

    for (int i = 0; i < COLS; i++)
        cleanup_gpio_line(&cols[i]);

    return -1;
}

static void keypad_cleanup(void)
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

static char keypad_scan(void)
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

/* Return one key per press, waiting for release to avoid repeats. */
static char keypad_get_key(void)
{
    while (running) {
        char key = keypad_scan();

        if (key != '\0') {
            delay_ms(25);

            if (keypad_scan() == key) {
                while (running && keypad_scan() != '\0')
                    delay_ms(10);

                return key;
            }
        }

        delay_ms(10);
    }

    return '\0';
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

static void display_confirmation(int study, int rest)
{
    char line[17];

    LCD_clear();
    LCD_setCursor(0, 0);
    snprintf(line, sizeof(line), "Study: %d min", study);
    LCD_printString(line);

    LCD_setCursor(1, 0);
    snprintf(line, sizeof(line), "Break: %d min", rest);
    LCD_printString(line);
}
        

static bool confirm_session(
    struct gpiod_line_request *button,
    int study,
    int rest)
{
    display_confirmation(study, rest);

    LCD_setCursor(0, 0);
    LCD_printString("SW=Start");

    LCD_setCursor(1, 0);
    LCD_printString("*=Back");

    printf("Press joystick SW to start or * on keypad to go back.\n");

    while (running) {

        // Check joystick button
        if (button_is_pressed(button)) {
            delay_ms(30);

            if (button_is_pressed(button)) {
                printf("SW BUTTON DETECTED!\n");
                fflush(stdout);

                while (running && button_is_pressed(button))
                    delay_ms(20);

                return running;
            }
        }

        // Non-blocking keypad check
        char key = keypad_scan();

        if (key == '*') {
            delay_ms(30);

            if (keypad_scan() == '*') {
                while (running && keypad_scan() != '\0')
                    delay_ms(10);

                return false;
            }
        }

        delay_ms(20);
    }

    return false;
}


/* ---------- Timer countdown ---------- */

static bool run_timer_phase(
    struct gpiod_line_request *button,
    const char *label,
    bool is_study,
    int minutes)
{
    (void)button;

    if (is_study)
        timer_study(0, minutes, 0);
    else
        timer_break(minutes, 0);

    while (running) {
        int hours = 0;
        int mins = 0;
        int seconds = 0;

        timer_update();
        TimerState state = get_state(&hours, &mins, &seconds);

        if (state == TIMER_IDLE)
            return true;

        LCD_clear();
        LCD_setCursor(0, 0);
        LCD_printString(label);

        char time_display[17];

        LCD_setCursor(1, 0);

        if (state == TIMER_PAUSED)
            snprintf(time_display, sizeof(time_display),
                     "PAUSED %02d:%02d", mins, seconds);
        else
            snprintf(time_display, sizeof(time_display),
                     "%02d:%02d:%02d", hours, mins, seconds);

        LCD_printString(time_display);

    
        delay_ms(100);
    }

    timer_idle();
    return false;
}


/* ---------- Complete study session ---------- */

static bool run_session(
    struct gpiod_line_request *button,
    int study_minutes,
    int break_minutes,
    bool is_pomodoro)
{
    (void)button;

    while (running) {
        if (is_pomodoro) {
            for (int round = 1; round <= 4 && running; round++) {
                char label[17];
                snprintf(label, sizeof(label), "STUDY ROUND %d", round);

                if (!run_timer_phase(NULL, label, true, study_minutes))
                    return false;

                if (round < 4) {
                    if (!run_timer_phase(NULL, "SHORT BREAK",
                                         false, break_minutes))
                        return false;
                } else {
                    if (!run_timer_phase(NULL, "LONG BREAK", false, 15))
                        return false;
                }
            }
        } else {
            if (!run_timer_phase(NULL, "STUDY TIME", true, study_minutes))
                return false;

            if (!run_timer_phase(NULL, "BREAK TIME", false, break_minutes))
                return false;
        }
    }

    timer_idle();
    return false;


    LCD_clear();
    LCD_setCursor(0, 0);
    LCD_printString("Session complete");
    LCD_setCursor(1, 0);
    LCD_printString("Press SW: menu");

    printf("Session complete. Press joystick SW to return to menu.\\n");

    return wait_for_button_press(button);
}

static void display_custom_entry(
    const char *label, const char *digits)
{
    LCD_clear();

    LCD_setCursor(0, 0);
    LCD_printString(label);

    LCD_setCursor(1, 0);
    LCD_printString(digits[0] ? digits : "_");
}

/*
 * Enter a positive number of minutes.
 * '#' confirms, '*' clears, and '*' on an empty field cancels.
 */
static int keypad_enter_minutes(const char *prompt)
{
    char digits[MAX_DIGITS + 1] = "";
    int length = 0;

    display_custom_entry(prompt, digits);

    while (running) {
        char key = keypad_get_key();

        if (!running || key == '\0')
            return -1;

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

            if (minutes < 1 || minutes > 999) {
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
    if (confirm_session(button, study, rest)) {
        printf("Starting %s: study %d min, break %d min\n",
               menu_options[selected], study, rest);

        run_session(button, study, rest, is_pomodoro);
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

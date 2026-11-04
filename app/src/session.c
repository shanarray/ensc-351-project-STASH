
#define _POSIX_C_SOURCE 200809L

#include "session.h"
#include "hal/lcd.h"
#include "hal/timer.h"
#include "keypad.h"

#include <stdio.h>
#include <time.h>

static void delay_ms(long ms, volatile sig_atomic_t *running)
{
    struct timespec t = {
        .tv_sec = ms / 1000,
        .tv_nsec = (ms % 1000) * 1000000L
    };

    while (nanosleep(&t, &t) == -1 && *running) {}
}

static bool button_is_pressed(struct gpiod_line_request *button)
{
    enum gpiod_line_value value =
        gpiod_line_request_get_value(button, 33);

    return value == GPIOD_LINE_VALUE_INACTIVE;
}

void display_confirmation(int study, int rest)
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

bool confirm_session(
    struct gpiod_line_request *button,
    int study,
    int rest,
    volatile sig_atomic_t *running)
{
    display_confirmation(study, rest);

    LCD_setCursor(0, 0);
    LCD_printString("Press to Start");

    LCD_setCursor(1, 0);
    LCD_printString("*=Back");

    printf("Press joystick SW to start or * on keypad to go back.\n");

    while (*running) {
        if (button_is_pressed(button)) {
            delay_ms(30, running);

            if (button_is_pressed(button)) {
                while (*running && button_is_pressed(button))
                    delay_ms(20, running);

                return *running;
            }
        }

        char key = keypad_scan();

        if (key == '*') {
            delay_ms(30, running);

            if (keypad_scan() == '*') {
                while (*running && keypad_scan() != '\0')
                    delay_ms(10, running);

                return false;
            }
        }

        delay_ms(20, running);
    }

    return false;
}

static bool run_timer_phase(
    const char *label,
    bool is_study,
    int minutes,
    volatile sig_atomic_t *running)
{
    if (is_study)
        timer_study(0, minutes, 0);
    else
        timer_break(minutes, 0);

    while (*running) {
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

        delay_ms(100, running);
    }

    timer_idle();
    return false;
}

bool run_session(
    int study_minutes,
    int break_minutes,
    bool is_pomodoro,
    volatile sig_atomic_t *running)
{
    while (*running) {
        if (is_pomodoro) {
            for (int round = 1; round <= 4 && *running; round++) {
                char label[17];
                snprintf(label, sizeof(label), "STUDY ROUND %d", round);

                if (!run_timer_phase(
                        label, true, study_minutes, running))
                    return false;

                if (round < 4) {
                    if (!run_timer_phase(
                            "SHORT BREAK", false, break_minutes, running))
                        return false;
                } else {
                    if (!run_timer_phase(
                            "LONG BREAK", false, 15, running))
                        return false;
                }
            }
        } else {
            if (!run_timer_phase(
                    "STUDY TIME", true, study_minutes, running))
                return false;

            if (!run_timer_phase(
                    "BREAK TIME", false, break_minutes, running))
                return false;
        }
    }

    timer_idle();
    return false;
}

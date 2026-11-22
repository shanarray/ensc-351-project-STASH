#define _POSIX_C_SOURCE 200809L
#include "session.h"
#include "keypad.h"
#include "feedback_system.h"
#include "input_handler.h"
#include "hal/lcd.h"
#include "timer.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

#define BUTTON_LINE 33

static void delay_ms(long ms)
{
    struct timespec t = {ms / 1000, (ms % 1000) * 1000000L};
    nanosleep(&t, NULL);
}

static bool sw_pressed(struct gpiod_line_request *button)
{
    return gpiod_line_request_get_value(button, BUTTON_LINE) == GPIOD_LINE_VALUE_INACTIVE;
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

bool confirm_session(struct gpiod_line_request *button, int study, int rest,
                     volatile sig_atomic_t *running)
{
    display_confirmation(study, rest);
    delay_ms(1000);
    LCD_clear();
    LCD_setCursor(0, 0); LCD_printString("Press SW to start");
    LCD_setCursor(1, 0); LCD_printString("*=Back");
    while (*running) {
        input_handler_update();
        FeedbackSystem_update();
        if (sw_pressed(button)) {
            delay_ms(30);
            if (sw_pressed(button)) {
                while (*running && sw_pressed(button)) {
                    input_handler_update(); FeedbackSystem_update(); delay_ms(10);
                }
                return *running;
            }
        }
        if (keypad_scan() == '*') {
            delay_ms(30);
            if (keypad_scan() == '*') {
                while (*running && keypad_scan() != '\0') delay_ms(10);
                return false;
            }
        }
        delay_ms(10);
    }
    return false;
}

/* Handles automatic transitions and early Study/Break button changes. */
bool run_session_active(int study_minutes, int break_minutes, bool is_pomodoro,
                        volatile sig_atomic_t *running)
{
    int completed_studies = 0;
    int initial_h = 0, initial_m = 0, initial_s = 0;
    TimerState active_phase = get_state(&initial_h, &initial_m, &initial_s);
    /* Preserve the phase even if it expires on the first update. */
    TimerState last_state = TIMER_OFF;
    int last_seconds = -1;

    while (*running) {
        input_handler_update();
        FeedbackSystem_update();

        int h = 0, m = 0, sec = 0;
        TimerState state = get_state(&h, &m, &sec);

        if (state == TIMER_RESET || state == TIMER_OFF)
            return false;

        /* The timer's IDLE state means the current phase has finished. */
        if (state == TIMER_IDLE) {
            if (active_phase == TIMER_STUDY) {
                completed_studies++;
                int rest = (is_pomodoro && completed_studies % 4 == 0)
                             ? 15 : break_minutes;
                timer_break(rest, 0);
                active_phase = TIMER_BREAK;
            } else if (active_phase == TIMER_BREAK) {
                timer_study(0, study_minutes, 0);
                active_phase = TIMER_STUDY;
            } else {
                return false;
            }
            last_state = TIMER_OFF;
            last_seconds = -1;
            continue;
        }

        /* A button can switch phases without ending the session. */
        if (state == TIMER_STUDY || state == TIMER_BREAK) {
            if (state != active_phase) {
                active_phase = state;
                last_state = TIMER_OFF;
                last_seconds = -1;
            }
        }

        /* Pause leaves active_phase unchanged so Resume continues correctly. */
        int remaining = h * 3600 + m * 60 + sec;
        if (state != last_state || remaining != last_seconds) {
            char line[17];
            const char *label = "STUDY TIME";
            if (state == TIMER_PAUSED) label = "PAUSED";
            else if (active_phase == TIMER_BREAK)
                label = is_pomodoro && completed_studies > 0 &&
                        completed_studies % 4 == 0 ? "LONG BREAK" : "BREAK TIME";

            LCD_clear();
            LCD_setCursor(0, 0);
            LCD_printString(label);
            LCD_setCursor(1, 0);
            snprintf(line, sizeof(line), "%02d:%02d:%02d", h, m, sec);
            LCD_printString(line);
            last_state = state;
            last_seconds = remaining;
        }
        delay_ms(10);
    }
    return false;
}

bool run_session(int study_minutes, int break_minutes, bool is_pomodoro,
                 volatile sig_atomic_t *running)
{
    if (study_minutes < 1 || break_minutes < 1) return false;
    timer_study(0, study_minutes, 0);
    return run_session_active(study_minutes, break_minutes, is_pomodoro, running);
}

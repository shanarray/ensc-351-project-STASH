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

static bool phase(const char *label, bool study, int minutes,
                  volatile sig_atomic_t *running)
{
    if (study) timer_study(0, minutes, 0);
    else timer_break(minutes, 0);
    TimerState expected = study ? TIMER_STUDY : TIMER_BREAK;
    int last_seconds = -1;
    TimerState last_state = TIMER_OFF;
    while (*running) {
        input_handler_update();
        FeedbackSystem_update(); /* Sole timer_update() owner. */
        int h = 0, m = 0, s = 0;
        TimerState state = get_state(&h, &m, &s);
        if (state == TIMER_IDLE) return true;
        if (state == TIMER_RESET || state == TIMER_OFF) return false;
        if (state != expected && state != TIMER_PAUSED) return false;
        int total = h * 3600 + m * 60 + s;
        if (state != last_state || total != last_seconds) {
            char line[17];
            LCD_clear();
            LCD_setCursor(0, 0);
            LCD_printString(state == TIMER_PAUSED ? "PAUSED" : label);
            LCD_setCursor(1, 0);
            snprintf(line, sizeof(line), "%02d:%02d:%02d", h, m, s);
            LCD_printString(line);
            last_state = state;
            last_seconds = total;
        }
        delay_ms(10);
    }
    return false;
}

bool run_session(int study_minutes, int break_minutes, bool is_pomodoro,
                 volatile sig_atomic_t *running)
{
    if (is_pomodoro) {
        for (int round = 1; round <= 4 && *running; ++round) {
            char label[17];
            snprintf(label, sizeof(label), "STUDY ROUND %d", round);
            if (!phase(label, true, study_minutes, running)) return false;
            if (!phase(round < 4 ? "SHORT BREAK" : "LONG BREAK",
                       false, round < 4 ? break_minutes : 15, running)) return false;
        }
    } else {
        if (!phase("STUDY TIME", true, study_minutes, running)) return false;
        if (!phase("BREAK TIME", false, break_minutes, running)) return false;
    }
    timer_idle();
    FeedbackSystem_update();
    return *running;
}

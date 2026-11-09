
#define _POSIX_C_SOURCE 200809L

#include "input_handler.h"
#include "adc.h"
#include "timer.h"

#include <stdbool.h>
#include <stdio.h>
#include <time.h>

enum {
    BUTTON_STUDY,
    BUTTON_BREAK,
    BUTTON_PAUSE,
    BUTTON_RESET_POWER,
    BUTTON_COUNT
};

static const int adc_channels[BUTTON_COUNT] = {1, 2, 3, 4};

/* Adjust after checking actual ADC readings. */
#define ADC_PRESS_THRESHOLD 1000
#define ADC_RELEASE_THRESHOLD 1800
#define BUTTON_DEBOUNCE_MS 30
#define POWER_HOLD_MS 2000

typedef struct {
    bool candidate;
    bool pressed;
    long long changed_ms;
    long long pressed_ms;
    bool long_handled;
} ButtonState;

static ButtonState buttons[BUTTON_COUNT];
static bool system_power = false;
static TimerState previous_running = TIMER_IDLE;
static bool initialized = false;

static long long now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    return (long long)ts.tv_sec * 1000 +
           ts.tv_nsec / 1000000;
}

static void short_press(int button)
{
    int h, m, s;
    TimerState state;

    if (system_power) return;

    switch (button) {
    case BUTTON_STUDY:
        puts("STUDY button pressed");
        timer_study(0, STUDY_MINUTES, 0);
        previous_running = TIMER_STUDY;
        break;

    case BUTTON_BREAK:
        puts("BREAK button pressed");
        timer_break(BREAK_MINUTES, 0);
        previous_running = TIMER_BREAK;
        break;

    case BUTTON_PAUSE:
        state = get_state(&h, &m, &s);

        if (state == TIMER_PAUSED) {
            timer_resume();
            puts("Timer resumed");
        }
        else if (state == TIMER_STUDY ||
                 state == TIMER_BREAK) {
            previous_running = state;
            timer_pause();
            puts("Timer paused");
        }
        else {
            puts("No active timer to pause");
        }
        break;

    case BUTTON_RESET_POWER:
        timer_reset();
        previous_running = TIMER_IDLE;
        puts("Timer reset");
        break;

    default:
        break;
    }
}

static void toggle_power(void)
{
    int h, m, s;

    if (!system_power) {
        TimerState state = get_state(&h, &m, &s);

        if (state == TIMER_STUDY ||
            state == TIMER_BREAK) {
            previous_running = state;
        }

        system_power = true;
        timer_off();
        puts("POWER OFF (held > 2s)");
    }
    else {
        system_power = false;

        if (previous_running == TIMER_STUDY ||
            previous_running == TIMER_BREAK) {
            timer_on();
            timer_pause();
        }
        else {
            timer_idle();
        }

        puts("POWER ON (held > 2s)");
    }
}

static void update_button(int index, bool raw_pressed,
                          long long t)
{
    ButtonState *b = &buttons[index];

    if (raw_pressed != b->candidate) {
        b->candidate = raw_pressed;
        b->changed_ms = t;
    }

    if (b->candidate != b->pressed &&
        t - b->changed_ms >= BUTTON_DEBOUNCE_MS) {

        b->pressed = b->candidate;

        if (b->pressed) {
            b->pressed_ms = t;
            b->long_handled = false;
        }
        else if (!b->long_handled) {
            short_press(index);
        }
    }

    if (index == BUTTON_RESET_POWER &&
        b->pressed &&
        !b->long_handled &&
        t - b->pressed_ms >= POWER_HOLD_MS) {

        b->long_handled = true;
        toggle_power();
    }
}

void input_handler_update_at(long long t)
{
    int readings[BUTTON_COUNT];

    for (int i = 0; i < BUTTON_COUNT; i++) {
        readings[i] = Adc_readChannel(adc_channels[i]);

        if (readings[i] < 0) {
            static bool warned = false;

            if (!warned) {
                fputs("ADC button read failed\n", stderr);
                warned = true;
            }
            return;
        }
    }

    if (!initialized) {
        for (int i = 0; i < BUTTON_COUNT; i++) {
            buttons[i].candidate =
                readings[i] < ADC_PRESS_THRESHOLD;

            buttons[i].pressed = buttons[i].candidate;
            buttons[i].changed_ms = t;
            buttons[i].pressed_ms = t;
            buttons[i].long_handled = buttons[i].pressed;
        }

        initialized = true;
        return;
    }

    for (int i = 0; i < BUTTON_COUNT; i++) {
        bool pressed = buttons[i].pressed
            ? readings[i] < ADC_RELEASE_THRESHOLD
            : readings[i] < ADC_PRESS_THRESHOLD;

        update_button(i, pressed, t);
    }
}

void input_handler_update(void)
{
    input_handler_update_at(now_ms());
}



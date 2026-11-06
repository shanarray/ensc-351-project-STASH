/* input_handler.c */
#include "input_handler.h"

#include <stdbool.h>
#include <time.h>
#include <stdio.h>

#include "gpio_buttons.h"
#include "timer.h"

typedef struct {
    bool raw_pressed;       // last raw reading 
    bool stable_pressed;    // debounced state 
    long long changed_at;   // when raw_pressed last changed
    long long pressed_at;       // timestamp when button went down
    bool long_press_handled;    // prevents repeating long-press action while held
} ButtonDebounce;

// We need to keep track of the states 
static Button current_state = BTN_IDLE;
static Button prev_state = BTN_IDLE;
static bool system_power = false;

static ButtonDebounce btn[BTN_COUNT];

static long long now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

// Runs once per debounced press (not while the button is held). 

/////// WHERE  THE PRINTING STUFF IS CHANGE TO MATCH WITH LED SCREEN //////////////
// Handles short presses (instant click actions)
static void on_short_press(Button b) {

    // Power is off don't let the other buttons have an effect
    if(system_power){
        return;
    }

    int h, m, s;

    switch (b) {
        case BTN_STUDY:
            printf("STUDY button pressed\n");
            timer_study(0, STUDY_MINUTES, 0);
            current_state = BTN_STUDY;
            break;

        case BTN_BREAK:
            printf("BREAK button pressed\n");
            timer_break(BREAK_MINUTES, 0);
            current_state = BTN_BREAK;
            break;

        case BTN_PAUSE:
            printf("PAUSE button pressed\n");
            if (get_state(&h, &m, &s) == TIMER_PAUSED) {
                timer_resume();
                current_state = prev_state;
                printf("Timer resumed\n");
            } else if (get_state(&h, &m, &s) == TIMER_STUDY ||
                       get_state(&h, &m, &s) == TIMER_BREAK) {
                timer_pause();
                prev_state = current_state;
                current_state = BTN_PAUSE;
                printf("Timer paused\n");
            } else {
                printf("No active timer to pause\n");
            }
            break;

        case BTN_RESET:
            printf("RESET button pressed (Short tap)\n");
            timer_reset();
            current_state = BTN_IDLE;
            printf("Timer reset\n");
            break;

        default:
            printf("Unknown button\n");
            break;
    }
}

// Handles long holds (> 2 seconds) for power/reset button
static void on_long_press(Button b) {
    if (b != BTN_RESET) return;

    if (!system_power) {
        // Timer was running/paused turn off 
        system_power = true;
        timer_off();

        // The previous state is still saved can be called in another function
        // Save local tracking state
        if (current_state != BTN_IDLE && current_state != BTN_PAUSE) {
            prev_state = current_state;
        }

        current_state = BTN_IDLE;
        printf("POWER OFF (Held > 3s)\n");
    } 
    else {

        // When we come back from off state it immediately starts at pause
        // Timer was off turn back on to previous state
        system_power = false;
        timer_on();
        timer_pause();
        
        current_state = BTN_PAUSE;
        printf("POWER ON (Held > 3s)\n");
    }
}

void input_handler_update_at(long long t) {
    (void)t;
    long long current_time = now_ms();

    for (int b = 0; b < BTN_COUNT; b++) {
        bool pressed = !gpio_read_button((Button)b); // Active low assumption

        if (pressed && !btn[b].stable_pressed) {
            // Button just went down
            btn[b].stable_pressed = true;
            btn[b].pressed_at = current_time;
            btn[b].long_press_handled = false;
        } 
        else if (pressed && btn[b].stable_pressed) {

            // Check for 2-second long press
            if (!btn[b].long_press_handled && (current_time - btn[b].pressed_at >= 2000)) {
                on_long_press((Button)b);
                btn[b].long_press_handled = true; // Mark handled so it doesn't loop
            }
        } 
        else if (!pressed && btn[b].stable_pressed) {
            // Button was released
            btn[b].stable_pressed = false;

            // If it was a short press and long press wasn't triggered
            if (!btn[b].long_press_handled) {
                on_short_press((Button)b);
            }
        }
    }
}


void input_handler_update(void) {

    input_handler_update_at(now_ms());
}

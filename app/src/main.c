#define _DEFAULT_SOURCE
#define _XOPEN_SOURCE 500

#include <assert.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

#include "feedback.h"
#include "led.h"
#include "gpio_buttons.h"
#include "input_handler.h"
#include "timer.h"
#include "buzzer_gpio.h"
#include "feedback_system.h"

static volatile sig_atomic_t running = 1;
static void on_sigint(int s) { 
    (void)s; 
    running = 0; 
}

static const char *state_name(TimerState s) {
    switch (s) {
    case TIMER_IDLE:   return "IDLE";
    case TIMER_STUDY:  return "STUDY";
    case TIMER_BREAK:  return "BREAK";
    case TIMER_PAUSED: return "PAUSED";
    case TIMER_RESET:  return "RESET";
    }
    return "?";
}

// Optional startup self-test using a mock buzzer callback
static bool mock_buzzer_on;
static void buzzer_mock(bool on) { 
    mock_buzzer_on = on; 
}

static void run_feedback_tests(void) {
    Feedback_init(buzzer_mock);

    Feedback_update(TIMER_STUDY, false, 1000);
    assert(Led_getState(LED_GREEN) && !Led_getState(LED_RED) && !Led_getState(LED_YELLOW));

    Feedback_update(TIMER_PAUSED, false, 1010);
    assert(!Led_getState(LED_GREEN) && Led_getState(LED_YELLOW));

    Feedback_update(TIMER_BREAK, false, 1020);
    assert(Led_getState(LED_RED) && !Led_getState(LED_YELLOW));

    Feedback_update(TIMER_RESET, false, 1030);
    assert(!Led_getState(LED_RED) && !Led_getState(LED_GREEN) && !Led_getState(LED_YELLOW));
    assert(!mock_buzzer_on);

    Feedback_update(TIMER_IDLE, true, 2000);
    assert(mock_buzzer_on);
    Feedback_update(TIMER_IDLE, false, 2249);
    assert(mock_buzzer_on);
    Feedback_update(TIMER_IDLE, false, 2250);
    assert(!mock_buzzer_on);

    Feedback_cleanup();
    assert(!mock_buzzer_on);
    printf("PASS: Feedback and LED unit self-test\n");
}

int main(void) {
    signal(SIGINT, on_sigint);

    // 1. Initialize LEDs and run unit self-test
    Led_init();
    run_feedback_tests();

    // 2. Initialize real hardware buzzer and the feedback system
    if (Buzzer_init() != 0) {
        fprintf(stderr, "Failed to initialize buzzer hardware\n");
        Led_cleanup();
        return 1;
    }
    FeedbackSystem_init(Buzzer_write);

    // 3. Initialize buttons
    if (gpio_buttons_init() != 0) {
        FeedbackSystem_cleanup();
        Buzzer_cleanup();
        Led_cleanup();
        return 1;
    }

    printf("Buttons: GPIO24=STUDY  GPIO25=BREAK  GPIO22=PAUSE/RESUME  GPIO23=RESET\n");
    printf("Ctrl+C to quit.\n");

    TimerState prev_state = (TimerState)-1;
    int prev_s = -1, h, m, s;

    TimerState st = get_state(&h, &m, &s);
    printf("main sees state=%d\n", st);

    while (running) {
        input_handler_update();     // Read + debounce buttons, change timer state 
        timer_update();             // Expire finished timers 
        FeedbackSystem_update();    // Updates underlying feedback state and real buzzer timing

        st = get_state(&h, &m, &s);

        // Print when state changes or the second ticks over
        if (st != prev_state || s != prev_s) {
            printf("%-6s %02d:%02d:%02d\n", state_name(st), h, m, s);
            prev_state = st;
            prev_s = s;
        }

        usleep(10000);              // 10 ms poll loop
    }

    // 4. Clean up all resources on exit
    gpio_buttons_cleanup();
    FeedbackSystem_cleanup();
    Buzzer_cleanup();
    Led_cleanup();

    printf("Done.\n");
    return 0;
}
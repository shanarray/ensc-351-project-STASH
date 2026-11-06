#define _DEFAULT_SOURCE
#define _XOPEN_SOURCE 500

#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "buzzer_gpio.h"
#include "feedback_system.h"
#include "gpio_buttons.h"
#include "input_handler.h"
#include "timer.h"

static volatile sig_atomic_t running = 1;

static void on_sigint(int signum)
{
    (void)signum;
    running = 0;
}

static const char *state_name(TimerState state)
{
    switch (state) {
    case TIMER_IDLE: return "IDLE";
    case TIMER_STUDY: return "STUDY";
    case TIMER_BREAK: return "BREAK";
    case TIMER_PAUSED: return "PAUSED";
    case TIMER_RESET: return "RESET";
    default: return "?";
    }
}

int main(int argc, char **argv)
{
    bool beep_test = argc == 2 && strcmp(argv[1], "--buzzer-test") == 0;
    bool timer_test = argc == 2 && strcmp(argv[1], "--timer-test") == 0;
    if (argc > 1 && !beep_test && !timer_test) {
        fprintf(stderr, "Usage: %s [--buzzer-test|--timer-test]\n", argv[0]);
        return 1;
    }

    signal(SIGINT, on_sigint);

    if (Buzzer_init() != 0) {
        fprintf(stderr, "Buzzer initialization failed\n");
        return 1;
    }

    if (beep_test) {
        puts("Buzzer ON for one second");
        fflush(stdout);
        Buzzer_write(true);
        sleep(1);
        Buzzer_write(false);
        Buzzer_cleanup();
        puts("Buzzer test complete");
        return 0;
    }

    FeedbackSystem_init(Buzzer_write);

    if (!timer_test && gpio_buttons_init() != 0) {
        fprintf(stderr, "Button initialization failed\n");
        FeedbackSystem_cleanup();
        Buzzer_cleanup();
        return 1;
    }

    if (timer_test) {
        puts("Starting 5-second study timer");
        timer_study(0, 0, 5);
    } else {
        puts("Buttons: GPIO24=STUDY, GPIO25=BREAK, GPIO22=PAUSE, GPIO23=RESET");
    }
    puts("LEDs: GPIO14, GPIO15, GPIO18; buzzer: GPIO13");
    puts("Ctrl+C to exit");

    TimerState last_state = (TimerState)-1;
    int last_h = -1, last_m = -1, last_s = -1;
    bool saw_running = false;
    while (running) {
        if (!timer_test)
            input_handler_update();

        // FeedbackSystem_update() calls timer_update() exactly once.
        FeedbackSystem_update();

        int h, m, s;
        TimerState state = get_state(&h, &m, &s);
        if (state == TIMER_STUDY || state == TIMER_BREAK)
            saw_running = true;
        if (state != last_state || h != last_h || m != last_m || s != last_s) {
            printf("%-6s %02d:%02d:%02d\n", state_name(state), h, m, s);
            fflush(stdout);
            last_state = state;
            last_h = h;
            last_m = m;
            last_s = s;
        }
        if (timer_test && saw_running && state == TIMER_IDLE) {
            // Allow the feedback callback to finish its 250 ms alert.
            usleep(350000);
            FeedbackSystem_update();
            break;
        }
        usleep(10000);
    }

    if (!timer_test)
        gpio_buttons_cleanup();
    FeedbackSystem_cleanup();
    Buzzer_cleanup();
    puts("Done");
    return 0;
}

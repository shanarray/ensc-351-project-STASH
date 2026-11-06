#define _POSIX_C_SOURCE 200809L
#include "feedback_system.h"
#include "hal/buzzer_gpio.h"
#include "timer.h"
#include <stdio.h>
#include <time.h>

static void sleep_ms(long ms)
{
    struct timespec delay = {ms / 1000, (ms % 1000) * 1000000L};
    nanosleep(&delay, NULL);
}

int main(void)
{
    if (Buzzer_init() != 0) return 1;
    FeedbackSystem_init(Buzzer_write);
    puts("Starting 5-second study session...");
    timer_study(0, 0, 5);
    int h = 0, m = 0, s = 0;
    TimerState old = TIMER_IDLE;
    for (int i = 0; i < 700; i++) {
        FeedbackSystem_update();
        TimerState state = get_state(&h, &m, &s);
        if (state != old) {
            if (state == TIMER_IDLE) puts("Timer finished - buzzer should beep once.");
            old = state;
        }
        sleep_ms(10);
    }
    FeedbackSystem_cleanup();
    Buzzer_cleanup();
    puts("Done.");
    return 0;
}

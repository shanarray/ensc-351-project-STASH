#define _POSIX_C_SOURCE 200809L
#include "feedback_system.h"
#include "led.h"
#include "timer.h"
#include <stdint.h>
#include <time.h>

static uint64_t time_ms(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (uint64_t)t.tv_sec * 1000u + (uint64_t)t.tv_nsec / 1000000u;
}

void FeedbackSystem_init(FeedbackBuzzerWrite buzzer_write)
{
    Led_init();
    Feedback_init(buzzer_write);
}

void FeedbackSystem_update(void)
{
    int hours, minutes, seconds;
    TimerState before = get_state(&hours, &minutes, &seconds);

    timer_update();

    TimerState after = get_state(&hours, &minutes, &seconds);
    bool finished = (before == TIMER_STUDY || before == TIMER_BREAK) &&
                    after == TIMER_IDLE;

    Feedback_update(after, finished, time_ms());
}

void FeedbackSystem_cleanup(void)
{
    Feedback_cleanup();
    Led_cleanup();
}

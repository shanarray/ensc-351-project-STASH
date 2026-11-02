#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include "feedback.h"
#include "hal/led.h"

static bool leds[3];
static bool buzzer;
void Led_setState(LedColor color, bool state) { leds[color] = state; }
static void buzzer_write(bool on) { buzzer = on; }

static void check(bool green, bool red, bool yellow, bool buzz)
{
    assert(leds[LED_GREEN] == green);
    assert(leds[LED_RED] == red);
    assert(leds[LED_YELLOW] == yellow);
    assert(buzzer == buzz);
}

int main(void)
{
    Feedback_init(buzzer_write);
    check(false, false, false, false);
    Feedback_update(TIMER_STUDY, false, 1000);
    check(true, false, false, false);
    Feedback_update(TIMER_PAUSED, false, 1020);
    check(false, false, true, false);
    Feedback_update(TIMER_BREAK, false, 1040);
    check(false, true, false, false);
    Feedback_update(TIMER_IDLE, true, 2000);
    check(false, false, false, true);
    Feedback_update(TIMER_IDLE, false, 2249);
    check(false, false, false, true);
    Feedback_update(TIMER_IDLE, false, 2250);
    check(false, false, false, false);
    Feedback_update(TIMER_IDLE, false, 3000); // Reset/idle, no beep
    check(false, false, false, false);
    Feedback_cleanup();
    check(false, false, false, false);
    puts("PASS: Feedback simulation tests");
    return 0;
}

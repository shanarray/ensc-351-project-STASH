#include "feedback.h"
#include "hal/led.h"
#include <assert.h>
#include <stdio.h>

static bool buzzer_on;
static void buzzer_mock(bool on) { buzzer_on = on; }

int main(void)
{
    Led_init();
    Feedback_init(buzzer_mock);

    Feedback_update(TIMER_STUDY, false, 1000);
    assert(Led_getState(LED_GREEN) && !Led_getState(LED_RED) && !Led_getState(LED_YELLOW));

    Feedback_update(TIMER_PAUSED, false, 1010);
    assert(!Led_getState(LED_GREEN) && Led_getState(LED_YELLOW));

    Feedback_update(TIMER_BREAK, false, 1020);
    assert(Led_getState(LED_RED) && !Led_getState(LED_YELLOW));

    Feedback_update(TIMER_RESET, false, 1030);
    assert(!Led_getState(LED_RED) && !Led_getState(LED_GREEN) && !Led_getState(LED_YELLOW));
    assert(!buzzer_on);

    Feedback_update(TIMER_IDLE, true, 2000);
    assert(buzzer_on);
    Feedback_update(TIMER_IDLE, false, 2249);
    assert(buzzer_on);
    Feedback_update(TIMER_IDLE, false, 2250);
    assert(!buzzer_on);

    Feedback_cleanup();
    assert(!buzzer_on);
    puts("PASS: LED states and buzzer callback");
    return 0;
}

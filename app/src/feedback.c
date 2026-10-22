#include "feedback.h"
#include "hal/led.h"

#define ALERT_DURATION_MS UINT64_C(250)

static FeedbackBuzzerWrite buzzer_output = 0;
static bool alert_active = false;
static uint64_t alert_started_ms = 0;

// Send the buzzer signal if a hardware output is available.
static void set_buzzer(bool on)
{
    if (buzzer_output) {
        buzzer_output(on);
    }
}

void Feedback_init(FeedbackBuzzerWrite buzzer_write)
{
    buzzer_output = buzzer_write;
    alert_active = false;
    alert_started_ms = 0;

    // All outputs start off.
    Led_setState(LED_GREEN, false);
    Led_setState(LED_RED, false);
    Led_setState(LED_YELLOW, false);
    set_buzzer(false);
}

void Feedback_update(TimerState state, bool session_completed, uint64_t now_ms)
{
    // Show study, break, or paused mode with the matching LED.
    Led_setState(LED_GREEN, state == TIMER_STUDY);
    Led_setState(LED_RED, state == TIMER_BREAK);
    Led_setState(LED_YELLOW, state == TIMER_PAUSED);

    // Sound the buzzer for 250 ms after a completed session.
    if (session_completed) {
        alert_active = true;
        alert_started_ms = now_ms;
    }
    if (alert_active && now_ms - alert_started_ms >= ALERT_DURATION_MS) {
        alert_active = false;
    }
    set_buzzer(alert_active);
}

void Feedback_cleanup(void)
{
    // Leave the LEDs and buzzer off when the program exits.
    alert_active = false;
    set_buzzer(false);
    Led_setState(LED_GREEN, false);
    Led_setState(LED_RED, false);
    Led_setState(LED_YELLOW, false);
    buzzer_output = 0;
}

/* Logic-only integration test: no GPIO, I2C, audio or board required. */
#define _POSIX_C_SOURCE 200809L
#include "session.h"
#include "timer.h"
#include <assert.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <gpiod.h>

static TimerState state;
static int study_minutes_seen[8];
static int break_minutes_seen[8];
static int studies, breaks, feedback_ticks;
static volatile sig_atomic_t *active_running;
static int manual_switch;

TimerState get_state(int *h, int *m, int *s) { *h=0; *m=0; *s=0; return state; }
void timer_study(int h, int m, int s) { (void)h;(void)s; study_minutes_seen[studies++]=m; state=TIMER_STUDY; }
void timer_break(int m, int s) { (void)s; break_minutes_seen[breaks++]=m; state=TIMER_BREAK; }
void timer_idle(void) { state=TIMER_IDLE; }
void timer_reset(void) { state=TIMER_RESET; }
void input_handler_update(void) {
    if (manual_switch && feedback_ticks == 0 && state == TIMER_STUDY) {
        timer_break(7,0); /* early break uses chosen duration */
    }
}
void FeedbackSystem_update(void) {
    feedback_ticks++;
    if (!manual_switch) {
        if (feedback_ticks == 1 || feedback_ticks == 2) state=TIMER_IDLE;
        if (feedback_ticks == 3) *active_running=0;
    } else {
        if (feedback_ticks == 2) state=TIMER_IDLE;
        if (feedback_ticks == 3) *active_running=0;
    }
}
void LCD_clear(void) {}
void LCD_setCursor(uint8_t r, uint8_t c) {(void)r;(void)c;}
void LCD_printString(const char *s) {(void)s;}
char keypad_scan(void) {return '\0';}
enum gpiod_line_value gpiod_line_request_get_value(struct gpiod_line_request *r, unsigned int o)
{(void)r;(void)o;return GPIOD_LINE_VALUE_ACTIVE;}

static void reset_test(void) {
    state=TIMER_IDLE; studies=0; breaks=0; feedback_ticks=0; manual_switch=0;
}
int main(void) {
    volatile sig_atomic_t running=1;
    reset_test();
    active_running=&running;
    assert(!run_session(30,7,false,&running));
    assert(studies >= 2 && breaks >= 1);
    assert(study_minutes_seen[0]==30 && study_minutes_seen[1]==30);
    assert(break_minutes_seen[0]==7);
    puts("PASS: study 30 -> break 7 -> study 30");

    running=1;
    reset_test();
    manual_switch=1;
    active_running=&running;
    assert(!run_session(30,7,false,&running));
    assert(studies >= 2 && breaks >= 1);
    assert(break_minutes_seen[0]==7 && study_minutes_seen[1]==30);
    puts("PASS: early break 7 -> study 30");
    return 0;
}

#define _POSIX_C_SOURCE 200809L
#include "timer.h"
#include <assert.h>
#include <stdio.h>
#include <time.h>

int main(void)
{
    int h=0,m=0,s=0;
    timer_idle();
    assert(get_state(&h,&m,&s) == TIMER_IDLE);
    timer_study(0,0,2);
    assert(get_state(&h,&m,&s) == TIMER_STUDY);
    timer_pause();
    assert(get_state(&h,&m,&s) == TIMER_PAUSED);
    timer_resume();
    assert(get_state(&h,&m,&s) == TIMER_STUDY);
    timer_off();
    assert(get_state(&h,&m,&s) == TIMER_OFF);
    timer_on();
    assert(get_state(&h,&m,&s) == TIMER_STUDY);
    timer_reset();
    assert(get_state(&h,&m,&s) == TIMER_RESET);
    timer_idle();
    struct timespec d = {2, 100000000L};
    timer_break(0,1);
    nanosleep(&d, NULL);
    timer_update();
    assert(get_state(&h,&m,&s) == TIMER_IDLE);
    puts("PASS: timer states, pause/resume, power, reset, expiry");
    return 0;
}

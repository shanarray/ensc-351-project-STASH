////////////////////////////////////////////////////////////
////// A shared header file to use the timer states ///////
//////////////////////////////////////////////////////////

#ifndef TIMER_H_
#define TIMER_H_

// Struct to define the different state timer will be in
typedef enum {
    TIMER_IDLE,
    TIMER_STUDY,
    TIMER_BREAK,
    TIMER_PAUSED,
    TIMER_RESET
} TimerState;


void timer_study(int hours, int minutes, int seconds);
void timer_break(int minutes, int seconds);

void timer_pause(void);
void timer_idle(void);
void timer_resume(void);
void timer_update(void);
void timer_reset(void);

TimerState get_state(int *hours, int *minutes, int *seconds);

#endif

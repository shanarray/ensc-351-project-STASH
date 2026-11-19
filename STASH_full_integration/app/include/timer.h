#ifndef TIMER_H_
#define TIMER_H_

// This module is the timer state machine used throughout the modules
// Defines the different states the machine can enter 

// Struct to define the different state timer will be in
typedef enum {
    TIMER_OFF,
    TIMER_IDLE,
    TIMER_STUDY,
    TIMER_BREAK,
    TIMER_PAUSED,
    TIMER_RESET
} TimerState;

TimerState get_state(int *hours, int *minutes, int *seconds);

void timer_study(int hours, int minutes, int seconds);
void timer_break(int minutes, int seconds);

void timer_pause(void);
void timer_idle(void);
void timer_resume(void);
void timer_update(void);
void timer_reset(void);
void timer_off(void);
void timer_on(void);

#endif
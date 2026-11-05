#ifndef TIMER_TEST_INTERFACE_H
#define TIMER_TEST_INTERFACE_H
typedef enum { TIMER_IDLE, TIMER_STUDY, TIMER_BREAK, TIMER_PAUSED, TIMER_RESET } TimerState;
TimerState get_state(int *hours, int *minutes, int *seconds);
void timer_update(void);
#endif

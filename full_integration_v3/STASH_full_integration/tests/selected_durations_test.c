/* Tests that physical Study and Break buttons use selected minutes. */
#include "input_handler.h"
#include "timer.h"
#include <assert.h>
#include <stdio.h>
static int adc[5]={0,3000,3000,3000,3000};
static int study_m = -1, break_m = -1;
static TimerState state = TIMER_IDLE;
int Adc_readChannel(int ch) {return adc[ch];}
TimerState get_state(int *h,int *m,int *s){*h=0;*m=0;*s=0;return state;}
void timer_study(int h,int m,int s){(void)h;(void)s; study_m=m;state=TIMER_STUDY;}
void timer_break(int m,int s){(void)s;break_m=m;state=TIMER_BREAK;}
void timer_resume(void) {state=TIMER_STUDY;}
void timer_pause(void) {state=TIMER_PAUSED;}
void timer_reset(void) {state=TIMER_RESET;}
void timer_off(void) {state=TIMER_OFF;}
void timer_on(void) {state=TIMER_STUDY;}
void timer_idle(void) {state=TIMER_IDLE;}
static void press(int ch, long long *t){
    adc[ch]=0;
    input_handler_update_at(*t); *t+=40;
    input_handler_update_at(*t); *t+=40;
    adc[ch]=3000;
    input_handler_update_at(*t); *t+=40;
    input_handler_update_at(*t); *t+=40;
}
int main(void){
    long long t=100;
    input_handler_update_at(t);
    input_handler_set_durations(42,9);
    press(1,&t);
    assert(study_m==42);
    press(2,&t);
    assert(break_m==9);
    input_handler_set_durations(30,2);
    press(1,&t);
    assert(study_m==30);
    press(2,&t);
    assert(break_m==2);
    puts("PASS: physical Study/Break buttons use currently selected durations");
    return 0;
}

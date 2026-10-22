// Header file
#include "timer.h"
#include <time.h>
#include <stdio.h> 
#include <unistd.h>

#ifndef TIMER_H
#define TIMER_H

// Global variable for other functions to use
static long long target_end = 0; 
static long long remaining_time = 0;
static TimerState current_state = TIMER_IDLE;
static TimerState prev_state = TIMER_IDLE;

// Generic clock that calculates in Ms, for better accuracy 
static long long getTimeInMs(void)
{
    struct timespec spec;
    clock_gettime(CLOCK_MONOTONIC, &spec);

    long long seconds = spec.tv_sec;
    long long nano_seconds = spec.tv_nsec;

    return (seconds * 1000) + (nano_seconds / 1000000); 
}

TimerState get_state(int *hours, int *minutes, int *seconds){
    
    long long current_time = getTimeInMs();
    long long remaining_ms = 0;

    // To update the time that shows on the LED
    if(current_state == TIMER_STUDY || current_state == TIMER_BREAK){
        if(current_time >= target_end){
            remaining_ms = 0;
        }
        else{
            remaining_ms = target_end - current_time;
        }
    }

    // Since time moves when timer is paused, need to update remaining time
    // from the global function
    else if(current_state == TIMER_PAUSED){
        remaining_ms = remaining_time;
    }

    // If the state is in IDLE 
    else{
        remaining_ms = 0;
    }

    // Converts remaining time in proper time units
    long long total_sec = remaining_ms / 1000;
    *hours = total_sec / 3600;
    *minutes = (total_sec % 3600) / 60;
    *seconds = total_sec % 60; 

    return current_state;


}

void timer_study(int hours, int minutes, int seconds){
    
    // Duration of timer, must convert all variables in the same unit of time
    // Converting all variables into Ms
    long long duration_study =  (long long)hours * 3600000 +
                                (long long)minutes * 60000 +
                                (long long)seconds * 1000;
        
    target_end = getTimeInMs() + duration_study;
    current_state = TIMER_STUDY;

}

void timer_break(int minutes, int seconds){

    // Duration of timer, must convert all variables in the same unit of time
    // Converting all variables into Ms
    long long duration_break =  (long long)minutes * 60000 +
                                (long long)seconds * 1000;
        
    target_end = getTimeInMs() + duration_break;
    current_state = TIMER_BREAK;

}

void timer_update(void){

    long long current_time = getTimeInMs();

    // Continuously call this function inside a loop in main
    // Checks to see if the timer is still running 
    if((current_state == TIMER_STUDY || current_state == TIMER_BREAK) && current_time >= target_end){

        current_state = TIMER_IDLE;
    }

}

void timer_pause(void){

    long long current_time = getTimeInMs();

    // Will only be activated if there's an ongoing timer
    if((current_state == TIMER_STUDY || current_state == TIMER_BREAK) && current_time <= target_end){

            remaining_time = target_end - current_time;
            prev_state = current_state;
            current_state = TIMER_PAUSED;
    }
    else{
        // Test, user interface if they press pause button and there's no timer
        printf("No timer running, nothing to pause.\n");
    }

}

void timer_resume(void){

    // Only activates if timer is paused
    if(current_state == TIMER_PAUSED){

        target_end = getTimeInMs() + remaining_time;
        
        // Resume from the previous state before timer
        current_state = prev_state;
        
    }
    else{
        // Test, user interface if they press resume button and there's no timer
        printf("No timer running, nothing to resume.\n");
    }

}

void timer_idle(void){

    // Restart the clocks because there's no current session
    target_end = 0;
    remaining_time = 0;
    current_state = TIMER_IDLE;
    prev_state = TIMER_IDLE;

}

void timer_reset(void)
{
    target_end = 0;
    remaining_time = 0;
    current_state = TIMER_RESET;
}


#endif 

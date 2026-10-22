///////// PARTIAL MAIN FOR TIMER /////////////

#include <stdio.h>
#include <unistd.h> // For usleep
#include <stdbool.h>
#include "timer.h"

int main(void) {

    // Initialise the variables
    int study_s = 0;
    int study_m = 0;
    int study_s = 0;

    int break_m = 0;
    int break_s = 0;

    bool next_session_break = false;
    int sessionsCompleted = 0;
    int totalSessions = 0; 
    
    // From input from user decide the cases 
    printf("Enter study_s, study_m, study_s (separated by spaces): ");
    scanf("%d %d %d", &h, &m, &s);
    
    // Study timer preset options
    ///////// User switch cases NOT FINISHED ////////
  /// interface still needs to be made for user to choose optiona and enter if else statemenets
    if(){
        timer_study(0, 40, 0);
            break_m = 10;
            break_s = 0;
            study_m = 40;

    }
    elseif(){
        timer_study(1,0,0);
            break_m = 25;
            break_s = 0;
            study_h = 1;
    }
    elseif(){
        timer_study(0,25,0);
            break_m = 5;
            break_s = 0;
            study_m =25;

    }
    else{
        timer_study(&study_s,&study_m,&study_s);
            break_m = 10;
            break_s = 0; study_h = 0; study_m = 0; study_s = 0; 
    }
    
    // Do we want user to be able to have multiple sessions? eg multiple session of 25
    // if so do we decide that or is that user choice? 

    //Loop while we are in study mode
    while (1) {
        timer_update();

        // BUTTON GPIO GOES HERE !!!
        
        // Make object state to define the switch cases
        TimerState state = get_state(&study_s, &study_m, &study_s);

        switch (state){

            case TIMER_STUDY:

               printf( "%02d:%02d:%02d\n", study_s, study_m, study_s);

            break;

            case TIMER_BREAK:

                printf( "%02d:%02d:%02d\n", study_m, study_s);

            break;

            case TIMER_PAUSED:

                printf( "Paused %02d:%02d:%02d\n", study_s, study_m, study_s);


            break;

            case TIMER_IDLE:

                if(!nextSessionIsBreak){

                    // Timer has left study state and returned to idle
                    timer_break(break_m, break_s);
                    nextSessionIsBreak = true; 

                }
                else{
                    
                    sessionsCompleted ++;

                    if(sessionsCompleted >= totalSessions){
                        printf( "session complete!");
                        return 0;
                    }

                    timer_study(study_h,study_m, study_s)
                    nextSessionIsBreak = false;
    
                }

            break; 
        }
        
        // Sleep for 1 second to print every second
        usleep(100000);

        }
    }

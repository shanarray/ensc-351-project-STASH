
#include "hal/timer.h"

#include <stdio.h>
#include <unistd.h>

int main(void)
{
    int hours, minutes, seconds;

    printf("Starting 10-second study timer...\n");

    timer_study(0, 0, 10);

    while (1) {
        timer_update();

        TimerState state = get_state(
            &hours, &minutes, &seconds
        );

        if (state == TIMER_IDLE) {
            printf("Timer complete!\n");
            break;
        }

        printf("Study: %02d:%02d:%02d\n",
               hours, minutes, seconds);

        sleep(1);
    }

    return 0;
}

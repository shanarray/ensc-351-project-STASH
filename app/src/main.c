#include <stdio.h>
#include <unistd.h>

#include "hal/motion_sensor.h"
#include "hal/buzzer_gpio.h"

int main()
{
    bool previous_motion = false;

    if (MotionSensor_init() != 0) {
        return 1;
    }

    if (Buzzer_init() != 0) {
        MotionSensor_cleanup();
        return 1;
    }

    printf("Hardware test beginning...\n");

    while (1) {
        bool motion = MotionSensor_detected();

        if (motion && !previous_motion) {
            printf("Motion detected!\n");
            Buzzer_beep(2000, 400); // 2000 Hz tone for 400 ms
        }
        else if (!motion && previous_motion) {
            printf("Motion_stopped!\n");
        }

        previous_motion = motion;
        usleep(100000);
    }

    Buzzer_cleanup();
    MotionSensor_cleanup();

    return 0;
}



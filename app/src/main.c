#include <stdio.h>
#include <unistd.h>

#include "hal/motion_sensor.h"
#include "hal/buzzer_gpio.h"

#define CHECK_INTERVAL_MS 100
#define LEAVE_DELAY_SEC 30

int main()
{
    bool alarm_triggered = false;
    int no_motion_count = 0;

    if (MotionSensor_init() != 0) {
        return 1;
    }

    if (Buzzer_init() != 0) {
        MotionSensor_cleanup();
        return 1;
    }

    printf("Leave detection program starting...\n");

    while (1) {
        bool motion = MotionSensor_detected();

        if (motion) {
            if (no_motion_count > 0) {
                printf("Motion detected. Resetting timer.\n");
            }
            no_motion_count = 0;
            alarm_triggered = false;
        }
        else {
            if (no_motion_count == 0) {
                printf("No motion detected. Starting timer...\n");
            }

            no_motion_count++;

            if (no_motion_count >= (LEAVE_DELAY_SEC * 1000 / CHECK_INTERVAL_MS) && !alarm_triggered) {
                printf("No motion for %d seconds. Sounding buzzer!\n", LEAVE_DELAY_SEC);

                for (int i = 0; i < 3; i++) {
                    Buzzer_beep(2000, 250);
                    usleep(100000);
                }

                alarm_triggered = true;
            }
        }
        usleep(CHECK_INTERVAL_MS * 1000);
    }

    Buzzer_cleanup();
    MotionSensor_cleanup();

    return 0;
}



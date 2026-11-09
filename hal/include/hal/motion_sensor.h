// Header file for motion sensor hardware
#ifndef MOTION_SENSOR_H
#define MOTION_SENSOR_H

#include <stdbool.h>

// Initialize motion sensor
int MotionSensor_init(void);

// Check for motion detection
bool MotionSensor_detected(void);

// Cleanup motion sensor
void MotionSensor_cleanup(void);

#endif

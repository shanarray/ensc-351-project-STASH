#ifndef MOTION_SENSOR_H
#define MOTION_SENSOR_H
#include <stdbool.h>
int MotionSensor_init(void);
bool MotionSensor_detected(void);
void MotionSensor_cleanup(void);
#endif

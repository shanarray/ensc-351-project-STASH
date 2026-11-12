#ifndef MOTION_BUZZER_H
#define MOTION_BUZZER_H
#include <stdbool.h>
int MotionBuzzer_init(void);
void MotionBuzzer_write(bool on);

// added for buzzer beep
void MotionBuzzer_beep(int frequency, int duration_ms);

void MotionBuzzer_cleanup(void);

#endif
#ifndef FEEDBACK_H
#define FEEDBACK_H

#include <stdbool.h>
#include <stdint.h>
#include "hal/timer.h"

// Function provided by the hardware module to switch the buzzer on or off.
typedef void (*FeedbackBuzzerWrite)(bool on);

// Initialize outputs after Led_init(). Pass NULL if no buzzer is connected.
void Feedback_init(FeedbackBuzzerWrite buzzer_write);

// Update LEDs and buzzer from the timer state on each loop iteration.
// session_completed is true only when a running timer expires naturally.
// now_ms is the monotonic time in milliseconds.
void Feedback_update(TimerState state, bool session_completed, uint64_t now_ms);

// Switch off the outputs before shutting down the LED hardware.
void Feedback_cleanup(void);

#endif


// Header file for helper functions to be implemented in main.c

#ifndef STUDY_HELPERS_H
#define STUDY_HELPERS_H

#include <stdbool.h>

// Initializing systems
/*
- GPIO, buttons, joystick, rotary encoder, LEDS, LCD
- Timer state machine and input handler
- Keypad, audio system, motion sensor
*/
bool study_init(void);

// Executes single interation of application loop (~50 Hz)
/*
- Checking inputs for buttons, joystick, encoder, keypad
- Evaluates motion sensor status
- Updates timer countdown
- Refreshes LCD display, LEDs, and triggers (audio alerts)
*/
void study_iteration(void);

// Continuous event loop
void study_run(void);

// Clean up all hardware, displays, etc. safely
void study_cleanup(void);

#endif


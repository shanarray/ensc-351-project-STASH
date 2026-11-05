#include "hal/led.h"

static bool led_values[3];

void Led_init(void) {}
void Led_cleanup(void) {}
void Led_setState(LedColor color, bool state) { led_values[color] = state; }
bool Led_getState(LedColor color) { return led_values[color]; }

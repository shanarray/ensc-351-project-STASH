// hal/include/hal/led.h
#ifndef LED_H
#define LED_H

#include <stdbool.h>

typedef enum {
    LED_RED = 0,
    LED_YELLOW,
    LED_GREEN
} LedColor;

void Led_init(void);
void Led_cleanup(void);
void Led_setState(LedColor color, bool state);
bool Led_getState(LedColor color);

#endif
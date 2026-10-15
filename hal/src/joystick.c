// hal/src/joystick.c
#include "hal/joystick.h"
#include "hal/gpio.h"

#define PIN_JS_UP     12
#define PIN_JS_DOWN   16
#define PIN_JS_LEFT   20
#define PIN_JS_RIGHT  21
#define PIN_JS_CENTER 26

void Joystick_init(void) {
    GPIO_setDirection(PIN_JS_UP, GPIO_DIRECTION_IN);
    GPIO_setDirection(PIN_JS_DOWN, GPIO_DIRECTION_IN);
    GPIO_setDirection(PIN_JS_LEFT, GPIO_DIRECTION_IN);
    GPIO_setDirection(PIN_JS_RIGHT, GPIO_DIRECTION_IN);
    GPIO_setDirection(PIN_JS_CENTER, GPIO_DIRECTION_IN);
}

void Joystick_cleanup(void) {
    // Cleanup logic
}

JoystickDirection Joystick_getDirection(void) {
    if (!GPIO_read(PIN_JS_UP))    return JOYSTICK_UP;
    if (!GPIO_read(PIN_JS_DOWN))  return JOYSTICK_DOWN;
    if (!GPIO_read(PIN_JS_LEFT))  return JOYSTICK_LEFT;
    if (!GPIO_read(PIN_JS_RIGHT)) return JOYSTICK_RIGHT;
    if (!GPIO_read(PIN_JS_CENTER))return JOYSTICK_CENTER;
    
    return JOYSTICK_CENTER;
}
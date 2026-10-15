// hal/src/button.c
#include "hal/button.h"
#include "hal/gpio.h"

#define PIN_BTN_START 5
#define PIN_BTN_PAUSE 6
#define PIN_BTN_END   13

void Button_init(void) {
    GPIO_setDirection(PIN_BTN_START, GPIO_DIRECTION_IN);
    GPIO_setDirection(PIN_BTN_PAUSE, GPIO_DIRECTION_IN);
    GPIO_setDirection(PIN_BTN_END,   GPIO_DIRECTION_IN);
}

void Button_cleanup(void) {
    // Cleanup if needed
}

bool Button_isPressed(uint8_t buttonId) {
    // Assuming active-low buttons (pressed = 0/false, released = 1/true) or adjust based on your wiring
    return !GPIO_read(buttonId); 
}
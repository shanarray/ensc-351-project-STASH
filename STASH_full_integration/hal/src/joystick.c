#include "hal/joystick.h"
#include "hal/adc.h"

#define THRESHOLD_LOW 800
#define THRESHOLD_HIGH 3200

/* The main program owns Adc_init() and Adc_cleanup(). */
void Joystick_init(void) {}
void Joystick_cleanup(void) {}

JoystickDirection Joystick_getDirection(void)
{
    int y_val = Adc_readChannel(0);
    if (y_val < 0) return JOYSTICK_CENTER;
    if (y_val > THRESHOLD_HIGH) return JOYSTICK_UP;
    if (y_val < THRESHOLD_LOW) return JOYSTICK_DOWN;
    return JOYSTICK_CENTER;
}

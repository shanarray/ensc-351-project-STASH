#include "hal/joystick.h"
#include "hal/adc.h"
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

static bool is_initialized = false;

// ADC thresholds for 12-bit range (0 - 4095, center ~ 2048)
#define THRESHOLD_LOW  800
#define THRESHOLD_HIGH 3200

#define CH_X 0
#define CH_Y 1

void Joystick_init(void) {
    assert(!is_initialized);

    Adc_init();

    is_initialized = true;
}

void Joystick_cleanup(void) {
    assert(is_initialized);

    Adc_cleanup();

    is_initialized = false;
}

JoystickDirection Joystick_getDirection(void) {
    int x_val = Adc_readChannel(0);
    int y_val = Adc_readChannel(1);

   // Temporary debug print to inspect raw analog values
   // printf("[DEBUG ADC] CH0(X): %-4d | CH1(Y): %-4d\n", x_val, y_val);

if (y_val > THRESHOLD_HIGH)
 return JOYSTICK_UP;
if (y_val < THRESHOLD_LOW)  
return JOYSTICK_DOWN;
if (x_val < THRESHOLD_LOW)  
return JOYSTICK_LEFT;
if (x_val > THRESHOLD_HIGH) 
return JOYSTICK_RIGHT;

return JOYSTICK_CENTER;
}
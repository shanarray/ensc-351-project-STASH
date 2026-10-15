// hal/src/rotary_encoder.c
#include "hal/rotary_encoder.h"
#include "hal/gpio.h"

#define PIN_ENC_A 23
#define PIN_ENC_B 24
#define PIN_ENC_SW 25

void RotaryEncoder_init(void) {
    GPIO_setDirection(PIN_ENC_A, GPIO_DIRECTION_IN);
    GPIO_setDirection(PIN_ENC_B, GPIO_DIRECTION_IN);
    GPIO_setDirection(PIN_ENC_SW, GPIO_DIRECTION_IN);
}

void RotaryEncoder_cleanup(void) {
    // Cleanup code
}

int8_t RotaryEncoder_getDelta(void) {
    // Stub for quadrature decoding logic. 
    // Returns +1 for clockwise step, -1 for counter-clockwise, 0 for no movement.
    static bool last_a = false;
    bool current_a = GPIO_read(PIN_ENC_A);
    bool current_b = GPIO_read(PIN_ENC_B);
    
    int8_t delta = 0;
    if (current_a != last_a) {
        if (current_b != current_a) {
            delta = 1;
        } else {
            delta = -1;
        }
        last_a = current_a;
    }
    return delta;
}

bool RotaryEncoder_isPushButtonPressed(void) {
    return !GPIO_read(PIN_ENC_SW);
}
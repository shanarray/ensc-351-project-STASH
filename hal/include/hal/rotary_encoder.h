// hal/include/hal/rotary_encoder.h
#ifndef ROTARY_ENCODER_H
#define ROTARY_ENCODER_H

#include <stdbool.h>
#include <stdint.h>

void RotaryEncoder_init(void);
void RotaryEncoder_cleanup(void);
int8_t RotaryEncoder_getDelta(void);
bool RotaryEncoder_isPushButtonPressed(void);

#endif
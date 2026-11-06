// hal/include/hal/rotary_encoder.h
#ifndef _ROTARY_ENCODER_H
#define _ROTARY_ENCODER_H

#include <stdbool.h>
#include <stdint.h>

bool rotary_encoder_init(void);
int8_t rotary_encoder_get_delta(void);
bool rotary_encoder_is_button_pressed(void);
void rotary_encoder_cleanup(void);


#endif

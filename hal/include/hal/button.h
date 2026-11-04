
// Sample button module 
// Part of the Hardware Abstraction Layer (HAL) 


#ifndef BUTTON_H
#define BUTTON_H

#include <stdbool.h>
#include <stdint.h>

void Button_init(void);
bool Button_isPressed(uint8_t buttonId);
void Button_cleanup(void);

#endif

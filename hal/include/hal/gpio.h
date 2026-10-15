// hal/include/hal/gpio.h
#ifndef GPIO_H
#define GPIO_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    GPIO_DIRECTION_IN = 0,
    GPIO_DIRECTION_OUT
} GpioDirection;

void GPIO_init(void);
void GPIO_cleanup(void);
void GPIO_setDirection(uint8_t pin, GpioDirection direction);
void GPIO_write(uint8_t pin, bool value);
bool GPIO_read(uint8_t pin);

#endif
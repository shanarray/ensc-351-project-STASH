// hal/include/hal/joystick.h

#ifndef JOYSTICK_H

#define JOYSTICK_H



#include <stdbool.h>



typedef enum {

JOYSTICK_CENTER = 0,

JOYSTICK_UP,

JOYSTICK_DOWN,

JOYSTICK_LEFT,

JOYSTICK_RIGHT

} JoystickDirection;



void Joystick_init(void);

void Joystick_cleanup(void);

JoystickDirection Joystick_getDirection(void);



#endif
#ifndef JOYSTICK_H
#define JOYSTICK_H

typedef enum {
    JOYSTICK_CENTER = 0,
    JOYSTICK_UP,
    JOYSTICK_DOWN
} JoystickDirection;

void Joystick_init(void);
void Joystick_cleanup(void);
JoystickDirection Joystick_getDirection(void);
#endif

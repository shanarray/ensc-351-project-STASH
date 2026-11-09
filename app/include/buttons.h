#ifndef BUTTONS_H
#define BUTTONS_H
#include <stdbool.h>

typedef enum {
    BTN_NONE = 0,
    BTN_STUDY,
    BTN_BREAK,
    BTN_PAUSE,
    BTN_IDLE,
    BTN_POWER
} Button;
typedef struct {
    Button group;
    bool power;
} ButtonSample;
int buttons_read(ButtonSample *sample);
#endif

#ifndef GPIO_BUTTONS_H
#define GPIO_BUTTONS_H
/* gpio_buttons.h - reads the four push buttons (libgpiod v2).
 *
 * Returns the RAW level: true = HIGH, false = LOW.
 * Buttons are wired pin -> button -> GND, so pressed = LOW. */

#include <stdbool.h>

typedef enum {
    BTN_STUDY,    // start a study session   - GPIO24, physical pin 18 
    BTN_BREAK,    // start a break           - GPIO25, physical pin 22 
    BTN_PAUSE,    // pause / resume toggle   - GPIO22, physical pin 15 
    BTN_RESET,    // reset the timer         - GPIO23, physical pin 16 
    BTN_COUNT,
    BTN_IDLE      // starting state, no physical attachment just a placeholder
} Button;

/* Returns 0 on success, -1 on failure (a message is printed). */
int  gpio_buttons_init(void);
void gpio_buttons_cleanup(void);

bool gpio_read_button(Button b);

/* Kept so the old test_buttons.c still works. */
bool gpio_read_pause(void);
bool gpio_read_reset(void);

#endif

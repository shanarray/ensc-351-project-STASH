// gpio_buttons.c 
#include "gpio_buttons.h"

#include <gpiod.h>
#include <stdio.h>

static const char *const BUTTON_NAMES[BTN_COUNT] = {
    [BTN_STUDY] = "GPIO24",   /* physical pin 18 */
    [BTN_BREAK] = "GPIO25",   /* physical pin 22 */
    [BTN_PAUSE] = "GPIO22",   /* physical pin 15 */
    [BTN_RESET] = "GPIO23",   /* physical pin 16 */
};

#define MAX_CHIPS    10
#define CONSUMER     "study_timer"

static struct gpiod_line_request *reqs[BTN_COUNT];
static unsigned int offsets[BTN_COUNT];

static struct gpiod_line_request *request_line(struct gpiod_chip *chip,
                                               unsigned int offset, bool pull_up) {
    struct gpiod_line_settings *ls = gpiod_line_settings_new();
    struct gpiod_line_config *lc = gpiod_line_config_new();
    struct gpiod_request_config *rc = gpiod_request_config_new();
    struct gpiod_line_request *req = NULL;

    if (ls && lc && rc) {
        gpiod_line_settings_set_direction(ls, GPIOD_LINE_DIRECTION_INPUT);
        if (pull_up) gpiod_line_settings_set_bias(ls, GPIOD_LINE_BIAS_PULL_UP);
        gpiod_request_config_set_consumer(rc, CONSUMER);
        if (gpiod_line_config_add_line_settings(lc, &offset, 1, ls) == 0)
            req = gpiod_chip_request_lines(chip, rc, lc);
    }
    if (rc) gpiod_request_config_free(rc);
    if (lc) gpiod_line_config_free(lc);
    if (ls) gpiod_line_settings_free(ls);
    return req;
}

static struct gpiod_line_request *request_by_name(const char *name, unsigned int *offset_out) {
   
    char path[32];

    for (int i = 0; i < MAX_CHIPS; i++) {
        snprintf(path, sizeof(path), "/dev/gpiochip%d", i);
        struct gpiod_chip *chip = gpiod_chip_open(path);

        if (!chip) continue;

        int off = gpiod_chip_get_line_offset_from_name(chip, name);

        if (off >= 0) {

            struct gpiod_line_request *req = request_line(chip, (unsigned int)off, true);
            
            if (!req) {
                fprintf(stderr, "gpio: pull-up refused for %s, retrying without it\n", name);
                req = request_line(chip, (unsigned int)off, false);
            }
            
            gpiod_chip_close(chip);   // the request stays valid after this 
            if (req) {
                *offset_out = (unsigned int)off;
                printf("gpio: %s -> %s line %d\n", name, path, off);
                return req;
            }
            
            fprintf(stderr, "gpio: found %s on %s line %d but could not request it "
                            "(in use? try sudo)\n", name, path, off);
            return NULL;
        }
        
        gpiod_chip_close(chip);
    }
    
    fprintf(stderr, "gpio: no line named %s found (check gpioinfo)\n", name);
    return NULL;
}

int gpio_buttons_init(void) {

    for (int b = 0; b < BTN_COUNT; b++) {
        
        reqs[b] = request_by_name(BUTTON_NAMES[b], &offsets[b]);
        
        if (!reqs[b]) {
            gpio_buttons_cleanup();
            return -1;
        }
    }
    return 0;
}

void gpio_buttons_cleanup(void) {
    
    for (int b = 0; b < BTN_COUNT; b++) {
        
        if (reqs[b]) gpiod_line_request_release(reqs[b]);
        reqs[b] = NULL;
    }
}

/* If a read fails, report HIGH (= "not pressed") so an error can't look like
 * a button press. */
bool gpio_read_button(Button b) {
    
    if ((unsigned)b >= BTN_COUNT || !reqs[b]) return true;
    return gpiod_line_request_get_value(reqs[b], offsets[b]) != GPIOD_LINE_VALUE_INACTIVE;
}

bool gpio_read_pause(void) { return gpio_read_button(BTN_PAUSE); }
bool gpio_read_reset(void) { return gpio_read_button(BTN_RESET); }

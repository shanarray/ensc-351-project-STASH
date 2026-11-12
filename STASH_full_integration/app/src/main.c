#define _POSIX_C_SOURCE 200809L
#include "hal/adc.h"
#include "hal/buzzer_gpio.h"
#include "hal/joystick.h"
#include "hal/lcd.h"
#include "feedback_system.h"
#include "input_handler.h"
#include "keypad.h"
#include "session.h"
#include "timer.h"
#include <gpiod.h>
#include <stdbool.h>
#include <signal.h>
#include <stdio.h>
#include <time.h>

#define MENU_COUNT 7
#define SW_CHIP "/dev/gpiochip1"
#define SW_OFFSET 33

typedef struct {int study, rest; bool pomodoro;} Mode;
static const char *menu_names[MENU_COUNT] = {
    "Pomodoro 25/5", "Study 60 min", "Study 90 min",
    "Study 120 min", "Study 30 min", "Study 180 min", "Custom Session"
};
static const Mode modes[MENU_COUNT] = {
    {25,5,true}, {60,10,false}, {90,20,false},
    {120,30,false}, {30,5,false}, {180,30,false}, {0,0,false}
};
static volatile sig_atomic_t running = 1;
static struct gpiod_chip *sw_chip = NULL;
static struct gpiod_line_request *sw_req = NULL;

static void stop_program(int sig) {(void)sig; running = 0;}
static void delay_ms(long ms)
{
    struct timespec ts = {ms / 1000, (ms % 1000) * 1000000L};
    nanosleep(&ts, NULL);
}

static bool sw_pressed(void)
{
    return sw_req &&
        gpiod_line_request_get_value(sw_req, SW_OFFSET) == GPIOD_LINE_VALUE_INACTIVE;
}

static int sw_init(void)
{
    sw_chip = gpiod_chip_open(SW_CHIP);
    if (!sw_chip) return -1;
    struct gpiod_line_settings *s = gpiod_line_settings_new();
    struct gpiod_line_config *c = gpiod_line_config_new();
    struct gpiod_request_config *r = gpiod_request_config_new();
    unsigned int offset = SW_OFFSET;
    if (s && c && r &&
        gpiod_line_settings_set_direction(s, GPIOD_LINE_DIRECTION_INPUT) == 0 &&
        gpiod_line_settings_set_bias(s, GPIOD_LINE_BIAS_PULL_UP) == 0 &&
        gpiod_line_config_add_line_settings(c, &offset, 1, s) == 0) {
        gpiod_request_config_set_consumer(r, "stash-joystick-switch");
        sw_req = gpiod_chip_request_lines(sw_chip, r, c);
    }
    if (s) gpiod_line_settings_free(s);
    if (c) gpiod_line_config_free(c);
    if (r) gpiod_request_config_free(r);
    return sw_req ? 0 : -1;
}

static void sw_cleanup(void)
{
    if (sw_req) gpiod_line_request_release(sw_req);
    if (sw_chip) gpiod_chip_close(sw_chip);
    sw_req = NULL; sw_chip = NULL;
}

static void show_menu(int selected)
{
    LCD_clear();
    LCD_setCursor(0, 0); LCD_printString(">");
    LCD_printString(menu_names[selected]);
    LCD_setCursor(1, 0); LCD_printString(" ");
    LCD_printString(menu_names[(selected+1)%MENU_COUNT]);
}

/* Manual ADC button sessions: update feedback and LCD while the timer runs. */
static void show_manual_timer(void)
{
    int h = 0, m = 0, s = 0, last_total = -1;
    TimerState last_state = TIMER_OFF;
    while (running) {
        input_handler_update();
        FeedbackSystem_update();
        TimerState state = get_state(&h, &m, &s);
        if (state != TIMER_STUDY && state != TIMER_BREAK &&
            state != TIMER_PAUSED) break;
        int total = h*3600+m*60+s;
        if (total != last_total || state != last_state) {
            char line[17];
            LCD_clear();
            LCD_setCursor(0,0);
            LCD_printString(state == TIMER_STUDY ? "STUDY" :
                            state == TIMER_BREAK ? "BREAK" : "PAUSED");
            LCD_setCursor(1,0);
            snprintf(line, sizeof(line), "%02d:%02d:%02d", h,m,s);
            LCD_printString(line);
            last_total = total;
            last_state = state;
        }
        delay_ms(10);
    }
    /* Finish the 250ms buzzer alert before returning to the menu. */
    for (int n = 0; n < 30 && running; ++n) {
        FeedbackSystem_update();
        delay_ms(10);
    }
}

int main(void)
{
    signal(SIGINT, stop_program);
    signal(SIGTERM, stop_program);
    bool adc_ready = false, buzzer_ready = false;
    bool feedback_ready = false, keypad_ready = false;
    bool lcd_ready = false;
    int exit_code = 1;

    if (Adc_init() != 0) goto cleanup;
    adc_ready = true;
    if (Buzzer_init() != 0) goto cleanup;
    buzzer_ready = true;
    FeedbackSystem_init(Buzzer_write);
    feedback_ready = true;
    LCD_init();
    lcd_ready = true;
    Joystick_init(); /* Does not initialize ADC again. */
    if (sw_init() != 0) {fputs("Joystick SW init failed\n",stderr); goto cleanup;}
    if (keypad_init() != 0) {fputs("Keypad init failed\n",stderr); goto cleanup;}
    keypad_ready = true;
    timer_idle();

    int selected = 0;
    bool moving_last = false;
    bool sw_last = false;
    bool was_off = false;
    show_menu(selected);
    puts("STASH ready: joystick UP/DOWN, SW select, CH1-CH4 buttons");
    while (running) {
        input_handler_update();
        FeedbackSystem_update();
        int h=0, m=0, s=0;
        TimerState state = get_state(&h,&m,&s);
        if (state == TIMER_OFF) {
            if (!was_off) LCD_clear();
            was_off = true;
            delay_ms(10);
            continue;
        }
        if (was_off) {
            was_off = false;
            show_menu(selected);
        }
        if (state == TIMER_STUDY || state == TIMER_BREAK || state == TIMER_PAUSED) {
            show_manual_timer();
            show_menu(selected);
            continue;
        }
        JoystickDirection dir = Joystick_getDirection();
        bool moving = (dir == JOYSTICK_UP || dir == JOYSTICK_DOWN);
        if (moving && !moving_last) {
            selected = (selected + (dir == JOYSTICK_UP ? MENU_COUNT-1 : 1)) % MENU_COUNT;
            show_menu(selected);
        }
        moving_last = moving;
        bool sw = sw_pressed();
        if (sw && !sw_last) {
            delay_ms(30);
            if (sw_pressed()) {
                while (running && sw_pressed()) {
                    input_handler_update(); FeedbackSystem_update(); delay_ms(10);
                }
                if (!running) break;
                int study = modes[selected].study, rest = modes[selected].rest;
                if (selected == MENU_COUNT-1) {
                    study = keypad_enter_minutes("Study minutes:");
                    if (study < 0) {show_menu(selected); sw_last = false; continue;}
                    rest = keypad_enter_minutes("Break minutes:");
                    if (rest < 0) {show_menu(selected); sw_last = false; continue;}
                }
                if (confirm_session(sw_req,study,rest,&running))
                    run_session(study,rest,modes[selected].pomodoro,&running);
                show_menu(selected);
            }
        }
        sw_last = sw;
        delay_ms(10);
    }
    exit_code = 0;
cleanup:
    if (keypad_ready) keypad_cleanup();
    sw_cleanup();
    Joystick_cleanup();
    if (lcd_ready) LCD_cleanup();
    if (feedback_ready) FeedbackSystem_cleanup();
    if (buzzer_ready) Buzzer_cleanup();
    if (adc_ready) Adc_cleanup();
    return exit_code;
}

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>

#include "study_helpers.h"
#include "hal/button.h"
#include "hal/gpio.h"
#include "hal/input_handler.h"
#include "hal/joystick.h"
#include "hal/lcd.h"
#include "hal/led.h"
#include "hal/rotary_encoder.h"
#include "hal/timer.h"

// Hardware pin definitions
#define PIN_ENC_A 23
#define PIN_ENC_B 24
#define PIN_ENC_SW 25
#define PIN_BTN_START 5
#define PIN_BTN_PAUSE 6
#define PIN_BTN_END 13

// FUTURE IMPLEMENTATIONS ***TO DO***
// Keypad, motion sensor, audio

// Static instance of input handler
static InputHandler input_handler_inst;

// Adapter functions for input pin readers
static bool read_encoder_a(void) 
{
    return GPIO_read(PIN_ENC_A);
}

static bool read_encoder_b(void)
{
    return GPIO_read(PIN_ENC_B);
}

static bool read_encoder_sw(void) 
{
    return !GPIO_read(PIN_ENC_SW);
}

static bool read_pause_btn(void) 
{
    return Button_isPressed(PIN_BTN_PAUSE);
}

static bool read_reset_btn(void)
{
    return Button_isPressed(PIN_BTN_END);
}

static JoyDir read_joystick(void)
{
    JoystickDirection d = Joystick_getDirection();
    
    if (d == JOYSTICK_UP)
        return JOY_UP;
    
    if (d == JOYSTICK_DOWN)
        return JOY_DOWN;

    if (d == JOYSTICK_LEFT)
        return JOY_LEFT;

    if (d == JOYSTICK_RIGHT)
        return JOY_RIGHT;

    if (d == JOYSTICK_CENTER)
        return JOY_PRESS;

    return JOY_NONE;
}

bool study_init(void) 
{
    printf("Initializing study session timer...\n");

    // Initialize low-level hardware modules
    GPIO_init();
    Button_init();
    Joystick_init();
    RotaryEncoder_init();
    Led_init();

    // Initialize LCD display
    LCD_init();
    LCD_clear();
    LCD_setCursor(0, 0);
    LCD_printString("Study Session Timer");

    // Set initial default LED state
    Led_setState(LED_GREEN, true);
    Led_setState(LED_RED, false);
    Led_setState(LED_YELLOW, false);

    // Setup input handler
    PinReaders readers = {
        .encoder_a = read_encoder_a,
        .encoder_b = read_encoder_b,
        .encoder_sw = read_encoder_sw,
        .pause_btn = read_pause_btn,
        .reset_btn = read_reset_btn,
        .joystick = read_joystick
    };
    input_init(&input_handler_inst, readers, NULL);

    // Initialize timer state to idle
    timer_idle();

    printf("Initialization complete.\n");
    return true;
}

void study_iteration(void) 
{
    Command cmds[10];
    int num_cmds = input_poll(&input_handler_inst, cmds, 10);

    // Process user inputs and commands from input handler
    for (int i = 0; i < num_cmds; i++) {
        switch (cmds[i]) {
            case CMD_START: {
                InputMode m = input_get_mode(&input_handler_inst);
                int mins = input_get_minutes(&input_handler_inst, m);
                if (m == MODE_STUDY) {
                    timer_study(0, mins, 0);
                    Led_setState(LED_GREEN, true);
                    Led_setState(LED_RED, false);
                }
                else {
                    timer_break(mins, 0);
                    Led_setState(LED_GREEN, false);
                    Led_setState(LED_RED, true);
                }
                printf("Started %s session for %d minutes.\n", (m == MODE_STUDY) ? "study" : "break", mins);
                break;
            }

            case CMD_TOGGLE_PAUSE: {
                int h, m, s;
                TimerState current = get_state(&h, &m, &s);
                if (current == TIMER_PAUSED) {
                    timer_resume();
                    printf("Timer resumed.\n");
                }
                else {
                    timer_pause();
                    printf("Timer paused.\n");
                }
                break;
            }

            case CMD_RESET:
                timer_idle();
                Led_setState(LED_YELLOW, false);
                Led_setState(LED_GREEN, true);
                Led_setState(LED_RED, false);
                printf("Timer reseted.\n");
                break;

            case CMD_MODE_CHANGED: {
                InputMode m = input_get_mode(&input_handler_inst);
                LCD_setCursor(1, 0);
                if (m == MODE_STUDY) {
                    LCD_printString("Mode: STUDY    ");
                }
                else {
                    LCD_printString("Mode: BREAK    ");
                }
                break;
            }

            case CMD_DURATION_CHANGED: {
                InputMode m = input_get_mode(&input_handler_inst);
                int mins = input_get_minutes(&input_handler_inst, m);
                printf("Duration updated via encoder: %d mins \n", mins);
                break;
            }
            default:
                break;
        }
    }

    // Update timer countdown logic
    timer_update();

    // Refresh LCD display with current remaining time
    int hours;
    int minutes;
    int seconds;

    TimerState state = get_state(&hours, &minutes, &seconds);

    char time_str[32];
    snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d", hours, minutes, seconds);

    LCD_setCursor(0, 0);
    LCD_printString(time_str);

    // Yellow LED on if paused
    if (state == TIMER_PAUSED) {
        Led_setState(LED_YELLOW, true);
    }
    else {
        led_setState(LED_YELLOW, false);
    }
}

void study_run(void) {
    printf("Entering main execution loop (~50 Hz)...\n");
    while (true) {
        study_iteration();
        usleep(20000); // 20 ms tick rate = 50 updates/sec
    }
}

void study_cleanup(void) {
    printf("Shutting down...\n");

    RotaryEncoder_cleanup();
    Led_cleanup();
    LCD_cleanup();
    Joystick_cleanup();
    Button_cleanup();
    GPIO_cleanup();

    printf("Cleanup complete.\n");
}


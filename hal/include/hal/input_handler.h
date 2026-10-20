#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    CMD_START,            /* encoder button pressed: start the selected mode */
    CMD_TOGGLE_PAUSE,     /* pause/resume button */
    CMD_RESET,            /* reset button */
    CMD_MODE_CHANGED,     /* joystick changed mode: read input_get_mode() */
    CMD_DURATION_CHANGED  /* encoder turned: read the new minutes */
} Command;

typedef enum { MODE_STUDY, MODE_BREAK, MODE_COUNT } InputMode;

typedef enum { JOY_NONE, JOY_UP, JOY_DOWN, JOY_LEFT, JOY_RIGHT, JOY_PRESS } JoyDir;

typedef bool   (*PinReadFn)(void);
typedef JoyDir (*JoyReadFn)(void);

typedef struct {
    PinReadFn encoder_a;   
    PinReadFn encoder_b;   
    PinReadFn encoder_sw;  
    PinReadFn pause_btn;   
    PinReadFn reset_btn;   
    JoyReadFn joystick;    /* pins TBD */
} PinReaders;

typedef struct {
    uint32_t debounce_ms;
    int step_minutes;                 /* minutes per encoder click */
    int minutes[MODE_COUNT];          /* current duration per mode */
    int min_minutes[MODE_COUNT];
    int max_minutes[MODE_COUNT];
} InputConfig;

typedef struct {
    bool stable;        /* debounced state, true = pressed */
    bool candidate;     /* most recent raw reading */
    uint64_t since_ms;  /* when candidate last changed */
} Button;

typedef struct {
    PinReaders rd;
    InputConfig cfg;
    InputMode mode;
    bool adjust_enabled;
    Button sw, pause, reset;
    JoyDir joy_stable, joy_candidate;
    uint64_t joy_since_ms;
    int prev_ab;      /* previous 2-bit encoder state */
    int quad_accum;   /* partial steps toward one detent */
} InputHandler;

InputConfig input_default_config(void);

void input_init(InputHandler *h, PinReaders readers, const InputConfig *cfg);

int input_poll(InputHandler *h, Command *out, int max_out);

int input_poll_at(InputHandler *h, uint64_t now_ms, Command *out, int max_out);

void input_set_adjust_enabled(InputHandler *h, bool enabled);

InputMode input_get_mode(const InputHandler *h);
int input_get_minutes(const InputHandler *h, InputMode mode);
int input_get_study_minutes(const InputHandler *h);
int input_get_break_minutes(const InputHandler *h);

#endif 

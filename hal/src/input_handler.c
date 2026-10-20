#define _POSIX_C_SOURCE 199309L
#include "input_handler.h"

#include <time.h>

static const int8_t QUAD_TABLE[16] = {
     0, -1,  1,  0,
     1,  0,  0, -1,
    -1,  0,  0,  1,
     0,  1, -1,  0
};

#define STEPS_PER_DETENT 4 

static uint64_t now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts); 
    return (uint64_t)ts.tv_sec * 1000u + (uint64_t)ts.tv_nsec / 1000000u;
}

static int clamp(int v, int lo, int hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

static void push(Command *out, int *n, int max, Command c) {
    if (*n < max) out[(*n)++] = c;
}

InputConfig input_default_config(void) {
    InputConfig c = {
        .debounce_ms = 20, .step_minutes = 1,
        .minutes     = { [MODE_STUDY] = 25,  [MODE_BREAK] = 5  },
        .min_minutes = { [MODE_STUDY] = 1,   [MODE_BREAK] = 1  },
        .max_minutes = { [MODE_STUDY] = 120, [MODE_BREAK] = 60 }
    };
    return c;
}

void input_init(InputHandler *h, PinReaders readers, const InputConfig *cfg) {
    uint64_t t = now_ms();
    h->rd = readers;
    h->cfg = cfg ? *cfg : input_default_config();
    h->mode = MODE_STUDY;
    h->adjust_enabled = true;
    h->quad_accum = 0;


    h->prev_ab = (h->rd.encoder_a() ? 2 : 0) | (h->rd.encoder_b() ? 1 : 0);
    h->sw.stable    = h->sw.candidate    = !h->rd.encoder_sw();
    h->pause.stable = h->pause.candidate = !h->rd.pause_btn();
    h->reset.stable = h->reset.candidate = !h->rd.reset_btn();
    h->sw.since_ms = h->pause.since_ms = h->reset.since_ms = t;
    h->joy_stable = h->joy_candidate = h->rd.joystick();
    h->joy_since_ms = t;
}

/* Returns true if the debounced state changed on this call. */
static bool debounce(Button *b, bool pressed_raw, uint64_t t, uint32_t debounce_ms) {
    if (pressed_raw != b->candidate) {   /* reading changed: restart the timer */
        b->candidate = pressed_raw;
        b->since_ms = t;
    }
    if (b->candidate != b->stable && t - b->since_ms >= debounce_ms) {
        b->stable = b->candidate;        /* held steady long enough: accept it */
        return true;
    }
    return false;
}

/* Same idea for the joystick, but the "state" is a direction, not on/off.
 * Returns true if the debounced direction changed on this call. */
static bool debounce_joy(InputHandler *h, JoyDir raw, uint64_t t) {
    if (raw != h->joy_candidate) {
        h->joy_candidate = raw;
        h->joy_since_ms = t;
    }
    if (h->joy_candidate != h->joy_stable &&
        t - h->joy_since_ms >= h->cfg.debounce_ms) {
        h->joy_stable = h->joy_candidate;
        return true;
    }
    return false;
}

/* Returns -1, 0 or +1 (one full detent). */
static int decode_encoder(InputHandler *h) {
    int ab = (h->rd.encoder_a() ? 2 : 0) | (h->rd.encoder_b() ? 1 : 0);
    h->quad_accum += QUAD_TABLE[(h->prev_ab << 2) | ab];
    h->prev_ab = ab;

    if (h->quad_accum >= STEPS_PER_DETENT)  { h->quad_accum = 0; return +1; }
    if (h->quad_accum <= -STEPS_PER_DETENT) { h->quad_accum = 0; return -1; }
    return 0;
}

/* Encoder only ever edits the time of the mode the joystick selected. */
static void adjust(InputHandler *h, int detents) {
    InputMode m = h->mode;
    h->cfg.minutes[m] = clamp(h->cfg.minutes[m] + detents * h->cfg.step_minutes,
                              h->cfg.min_minutes[m], h->cfg.max_minutes[m]);
}

int input_poll(InputHandler *h, Command *out, int max_out) {
    return input_poll_at(h, now_ms(), out, max_out);
}

int input_poll_at(InputHandler *h, uint64_t t, Command *out, int max_out) {
    int n = 0;

    /* 1. Joystick: choose the mode. Acts once per push; 
     *    the stick must return to centre before it can move
     *    again, so holding it does not spin through the modes. */
    if (debounce_joy(h, h->rd.joystick(), t) && h->adjust_enabled) {
        if (h->joy_stable == JOY_RIGHT || h->joy_stable == JOY_DOWN) {
            h->mode = (InputMode)((h->mode + 1) % MODE_COUNT);
            push(out, &n, max_out, CMD_MODE_CHANGED);
        } else if (h->joy_stable == JOY_LEFT || h->joy_stable == JOY_UP) {
            h->mode = (InputMode)((h->mode + MODE_COUNT - 1) % MODE_COUNT);
            push(out, &n, max_out, CMD_MODE_CHANGED);
        }
        /* JOY_PRESS / JOY_NONE: nothing (centre press is reserved) */
    }

    /* 2. Encoder rotation: set the custom time. Always decode (so we never lose
     *    position), but only apply the change when editing is allowed. */
    int d = decode_encoder(h);
    if (d != 0 && h->adjust_enabled) {
        adjust(h, d);
        push(out, &n, max_out, CMD_DURATION_CHANGED);
    }

    /* 3. Buttons: act only on the debounced press edge, so holding a button
     *    does not repeat the action. */
    if (debounce(&h->sw, !h->rd.encoder_sw(), t, h->cfg.debounce_ms) && h->sw.stable)
        push(out, &n, max_out, CMD_START);
    if (debounce(&h->pause, !h->rd.pause_btn(), t, h->cfg.debounce_ms) && h->pause.stable)
        push(out, &n, max_out, CMD_TOGGLE_PAUSE);
    if (debounce(&h->reset, !h->rd.reset_btn(), t, h->cfg.debounce_ms) && h->reset.stable)
        push(out, &n, max_out, CMD_RESET);

    return n;
}

void input_set_adjust_enabled(InputHandler *h, bool enabled) { h->adjust_enabled = enabled; }
InputMode input_get_mode(const InputHandler *h) { return h->mode; }
int input_get_minutes(const InputHandler *h, InputMode m) { return h->cfg.minutes[m]; }
int input_get_study_minutes(const InputHandler *h) { return h->cfg.minutes[MODE_STUDY]; }
int input_get_break_minutes(const InputHandler *h) { return h->cfg.minutes[MODE_BREAK]; }

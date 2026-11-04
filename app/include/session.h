#ifndef SESSION_H
#define SESSION_H

#include <gpiod.h>
#include <stdbool.h>
#include <signal.h>

bool confirm_session(
    struct gpiod_line_request *button,
    int study,
    int rest,
    volatile sig_atomic_t *running
);

bool run_session(
    int study_minutes,
    int break_minutes,
    bool is_pomodoro,
    volatile sig_atomic_t *running
);

void display_confirmation(int study, int rest);

#endif

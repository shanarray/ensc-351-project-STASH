#define _POSIX_C_SOURCE 200809L
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <time.h>
#include <unistd.h>
#include "feedback.h"
#include "hal/led.h"
#include "hal/timer.h"

static uint64_t monotonic_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
}

static const char *state_name(TimerState state)
{
    switch (state) {
        case TIMER_STUDY: return "STUDY (green)";
        case TIMER_BREAK: return "BREAK (red)";
        case TIMER_PAUSED: return "PAUSED (yellow)";
        default: return "IDLE (off)";
    }
}

static void buzzer_test_output(bool on)
{
    static bool previous = false;

    if (on != previous) {
        puts(on ? "BUZZER ON" : "BUZZER OFF");
        previous = on;
    }
}

int main(void)
{
    char command[100];
    int hours = 0, minutes = 0, seconds = 0;
    TimerState previous = TIMER_IDLE;
    TimerState state;
    int last_seconds = -1;
    bool running = true;

    setvbuf(stdout, NULL, _IONBF, 0);
    Led_init();
    Feedback_init(buzzer_test_output);
    timer_idle();
    puts("Commands: study [seconds], break [seconds], pause, resume, reset, status, quit");
    puts("Try: study 15, pause, resume, break 8, reset");

    while (running) {
        fd_set rfds;
        struct timeval timeout = {0, 100000};
        FD_ZERO(&rfds);
        FD_SET(STDIN_FILENO, &rfds);
        int ready = select(STDIN_FILENO + 1, &rfds, NULL, NULL, &timeout);
        if (ready < 0) {
            perror("select");
            break;
        }
        bool reset = false;
        if (ready > 0 && FD_ISSET(STDIN_FILENO, &rfds)) {
            if (!fgets(command, sizeof(command), stdin)) break;
            int duration = 0;
            if (sscanf(command, "study %d", &duration) == 1 && duration > 0) {
                timer_study(0, 0, duration);
            } else if (strcmp(command, "study\n") == 0) {
                timer_study(0, 0, 15);
            } else if (sscanf(command, "break %d", &duration) == 1 && duration > 0) {
                timer_break(0, duration);
            } else if (strcmp(command, "break\n") == 0) {
                timer_break(0, 8);
            } else if (strcmp(command, "pause\n") == 0) {
                timer_pause();
            } else if (strcmp(command, "resume\n") == 0) {
                timer_resume();
            } else if (strcmp(command, "reset\n") == 0) {
                timer_idle();
                reset = true;
            } else if (strcmp(command, "quit\n") == 0) {
                running = false;
            } else if (strcmp(command, "status\n") != 0) {
                puts("Unknown command. Use study 15, break 8, pause, resume, reset, status, quit.");
            }
        }
        // The timer calculates the actual state and countdown.
        timer_update();
        state = get_state(&hours, &minutes, &seconds);
        bool completed = !reset &&
            (previous == TIMER_STUDY || previous == TIMER_BREAK) &&
            state == TIMER_IDLE;
        // The feedback module selects the physical LED outputs.
        Feedback_update(state, completed, monotonic_ms());
        int total_seconds = hours * 3600 + minutes * 60 + seconds;
        if (state != previous || (total_seconds != last_seconds && state != TIMER_IDLE)) {
            printf("%s, remaining %02d:%02d:%02d%s\n", state_name(state),
                   hours, minutes, seconds, completed ? " [session finished]" : "");
        }
        previous = state;
        last_seconds = total_seconds;
    }
    Feedback_cleanup();
    Led_cleanup();
    puts("LEDs off. Exiting.");
    return 0;
}

#define _POSIX_C_SOURCE 200809L
#include "web_status.h"
#include "extras.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const char *state_name(TimerState state)
{
    switch (state) {
    case TIMER_OFF: return "OFF";
    case TIMER_IDLE: return "IDLE";
    case TIMER_STUDY: return "STUDY";
    case TIMER_BREAK: return "BREAK";
    case TIMER_PAUSED: return "PAUSED";
    case TIMER_RESET: return "RESET";
    default: return "UNKNOWN";
    }
}

static const char *status_path(void)
{
    const char *path = getenv("STASH_STATUS_FILE");
    return path && path[0] ? path : NULL;
}

static long long monotonic_ms(void)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (long long)now.tv_sec * 1000 + now.tv_nsec / 1000000;
}

void WebStatus_publish(TimerState state, int hours, int minutes, int seconds)
{
    static long long last_write = -1;
    static TimerState last_state = (TimerState)-1;
    static int last_seconds = -1;
    const char *path = status_path();
    if (!path || strlen(path) > 900) return;

    int total = hours * 3600 + minutes * 60 + seconds;
    long long now = monotonic_ms();
    if (last_write >= 0 && state == last_state && total == last_seconds &&
        now - last_write < 1000) return;
    if (last_write >= 0 && state == last_state && now - last_write < 900) return;

    char temp[1024];
    snprintf(temp, sizeof(temp), "%s.tmp", path);
    FILE *file = fopen(temp, "w");
    if (!file) { perror("STASH: status open"); return; }

    int motion = Extras_motion_status();
    int volume = Extras_music_volume();
    int result = fprintf(file,
        "{\"state\":\"%s\",\"remaining_seconds\":%d,"
        "\"light\":null,\"noise\":null,\"motion\":",
        state_name(state), total);
    if (result >= 0) {
        if (motion < 0) result = fputs("null", file);
        else result = fputs(motion ? "true" : "false", file);
    }
    if (result >= 0) {
        if (volume < 0) result = fprintf(file, ",\"music_volume\":null}\n");
        else result = fprintf(file, ",\"music_volume\":%d}\n", volume);
    }
    int close_result = fclose(file);
    if (result < 0 || close_result != 0) {
        unlink(temp);
        return;
    }
    if (rename(temp, path) != 0) { perror("STASH: status rename"); unlink(temp); return; }
    last_write = now;
    last_state = state;
    last_seconds = total;
}

void WebStatus_log_session(const char *mode, int duration_seconds)
{
    const char *path = getenv("STASH_HISTORY_FILE");
    if (!path || !path[0] || !mode || duration_seconds <= 0) return;
    FILE *file = fopen(path, "a+");
    if (!file) { perror("STASH: history open"); return; }
    if (fseek(file, 0, SEEK_END) == 0 && ftell(file) == 0)
        fputs("date,mode,duration_seconds\n", file);
    time_t now = time(NULL);
    struct tm tm;
    char date[32] = "";
    if (localtime_r(&now, &tm))
        strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", &tm);
    fprintf(file, "%s,%s,%d\n", date, mode, duration_seconds);
    fclose(file);
}

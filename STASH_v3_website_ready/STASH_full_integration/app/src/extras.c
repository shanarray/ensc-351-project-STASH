#define _POSIX_C_SOURCE 200809L
#include "extras.h"
#include "hal/motion_sensor.h"
#include "hal/buzzer_gpio.h"
#include "hal/rotary_encoder.h"
#include "hal/audio.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifndef STASH_ENABLE_ROTARY_ENCODER
#define STASH_ENABLE_ROTARY_ENCODER 0
#endif
#define DEFAULT_WAV_FILE "/mnt/remote/home/debian/music/study_music.wav"
#define MOTION_CHECK_INTERVAL_MS 100
#define LEAVE_DELAY_MS 30000
#define BUTTON_DEBOUNCE_MS 250
#define VOLUME_STEP 0.05f
#define MAX_MUSIC_VOLUME 0.60f

static bool motion_ready, encoder_ready, audio_ready;
static int64_t last_motion_ms, last_motion_check_ms;
static bool alarm_triggered;
static bool motion_detected;
static float volume = 0.25f;
static bool audio_paused, previous_button;
static int64_t last_button_press_ms;

static int64_t now_ms(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (int64_t)t.tv_sec * 1000 + t.tv_nsec / 1000000;
}

void Extras_init(void)
{
    int64_t now = now_ms();
    last_motion_ms = now;
    last_motion_check_ms = now - MOTION_CHECK_INTERVAL_MS;
    last_button_press_ms = now - BUTTON_DEBOUNCE_MS;
    motion_ready = MotionSensor_init() == 0;
    if (!motion_ready) fputs("STASH: motion sensor unavailable; timer still works\n", stderr);

#if STASH_ENABLE_ROTARY_ENCODER
    encoder_ready = rotary_encoder_init();
    if (!encoder_ready) fputs("STASH: music encoder unavailable\n", stderr);
#else
    puts("STASH: rotary controls disabled");
#endif

    const char *wav = getenv("STASH_WAV_FILE");
    if (!wav || !wav[0]) wav = DEFAULT_WAV_FILE;
    audio_ready = audio_init(wav);
    if (audio_ready) {
        audio_set_volume(volume);
        if (!audio_play()) { audio_cleanup(); audio_ready = false; }
    }
    if (!audio_ready) fputs("STASH: music unavailable; timer still works\n", stderr);
}

void Extras_update(void)
{
    int64_t now = now_ms();
    if (motion_ready && now - last_motion_check_ms >= MOTION_CHECK_INTERVAL_MS) {
        last_motion_check_ms = now;
        motion_detected = MotionSensor_detected();
        if (motion_detected) {
            last_motion_ms = now;
            alarm_triggered = false;
        } else if (!alarm_triggered && now - last_motion_ms >= LEAVE_DELAY_MS) {
            alarm_triggered = true;
            puts("No motion for 30 seconds. Sounding alert.");
            Buzzer_motion_alert_start();
        }
    }
    Buzzer_update();
    if (!encoder_ready) return;
    int8_t delta = rotary_encoder_get_delta();
    if (delta && audio_ready) {
        volume += (float)delta * VOLUME_STEP;
        if (volume > MAX_MUSIC_VOLUME) volume = MAX_MUSIC_VOLUME;
        if (volume < 0.0f) volume = 0.0f;
        audio_set_volume(volume);
        printf("Volume: %.0f%%\n", volume * 100.0f);
    }
    bool pressed = rotary_encoder_is_button_pressed();
    if (audio_ready && pressed && !previous_button &&
        now - last_button_press_ms >= BUTTON_DEBOUNCE_MS) {
        last_button_press_ms = now;
        if (audio_paused) audio_resume();
        else audio_pause();
        audio_paused = !audio_paused;
        puts(audio_paused ? "Music paused" : "Music resumed");
    }
    previous_button = pressed;
}

void Extras_cleanup(void)
{
    if (audio_ready) audio_cleanup();
    audio_ready = false;
    if (encoder_ready) rotary_encoder_cleanup();
    encoder_ready = false;
    if (motion_ready) MotionSensor_cleanup();
    motion_ready = false;
}

int Extras_motion_status(void)
{
    return motion_ready ? (motion_detected ? 1 : 0) : -1;
}

int Extras_music_volume(void)
{
    return audio_ready ? (int)(volume * 100.0f + 0.5f) : -1;
}

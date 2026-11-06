
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <signal.h>
#include <time.h>

#include "hal/rotary_encoder.h"
#include "hal/audio.h"

#define WAV_FILE "/mnt/remote/home/debian/music/study_music.wav"

#define INITIAL_VOLUME 0.50f
#define VOLUME_STEP 0.05f
#define BUTTON_DEBOUNCE_MS 250

static volatile sig_atomic_t running = 1;

static void handle_signal(int signal_number)
{
    (void)signal_number;
    running = 0;
}

static long milliseconds_now(void)
{
    struct timespec now;

    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0)
        return 0;

    return now.tv_sec * 1000L + now.tv_nsec / 1000000L;
}

int main(void)
{
    float volume = INITIAL_VOLUME;
    bool paused = false;
    bool previous_button_pressed = false;
    long last_button_press_ms = -BUTTON_DEBOUNCE_MS;

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    if (!rotary_encoder_init()) {
        fprintf(stderr, "Failed to initialize rotary encoder\n");
        return EXIT_FAILURE;
    }

    if (!audio_init(WAV_FILE)) {
        fprintf(stderr, "Failed to initialize audio\n");
        rotary_encoder_cleanup();
        return EXIT_FAILURE;
    }

    audio_set_volume(volume);

    if (!audio_play()) {
        fprintf(stderr, "Failed to start audio playback\n");
        audio_cleanup();
        rotary_encoder_cleanup();
        return EXIT_FAILURE;
    }

    printf("Study timer music player started.\n");
    printf("Rotate: adjust volume | Press: pause/resume\n");
    printf("Ctrl+C: exit\n");
    printf("Initial volume: %.0f%%\n", volume * 100.0f);

    while (running) {
        int8_t delta = rotary_encoder_get_delta();

        if (delta != 0) {
            volume += (float)delta * VOLUME_STEP;

            if (volume > 1.0f)
                volume = 1.0f;
            if (volume < 0.0f)
                volume = 0.0f;

            audio_set_volume(volume);
            printf("Volume: %.0f%%\n", volume * 100.0f);
        }

        bool button_pressed =
            rotary_encoder_is_button_pressed();

        long now_ms = milliseconds_now();

        if (button_pressed && !previous_button_pressed &&
            now_ms - last_button_press_ms >= BUTTON_DEBOUNCE_MS) {

            last_button_press_ms = now_ms;

            if (paused) {
                audio_resume();
                paused = false;
                printf("Playback resumed\n");
            } else {
                audio_pause();
                paused = true;
                printf("Playback paused\n");
            }
        }

        previous_button_pressed = button_pressed;

        struct timespec delay = {
            .tv_sec = 0,
            .tv_nsec = 10000000L
        };

        (void)nanosleep(&delay, NULL);
    }

    printf("\nStopping player...\n");

    audio_cleanup();
    rotary_encoder_cleanup();

    printf("Shutdown complete.\n");
    return EXIT_SUCCESS;
}



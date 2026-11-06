
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <limits.h>
#include <errno.h>
#include <math.h>
#include <time.h>

#include <alsa/asoundlib.h>

#include "hal/audio.h"


// ALSA configuration 
#define ALSA_DEVICE "plughw:CARD=MAX98357A,DEV=0"
#define FRAMES_PER_BUFFER 1024
#define OUTPUT_SAMPLE_RATE 48000


// Audio resources 
static FILE *wav_file;
static snd_pcm_t *pcm_handle;
static pthread_t playback_thread;


// Thread synchronization 
static pthread_mutex_t audio_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t audio_cond = PTHREAD_COND_INITIALIZER;


// Audio state 
static bool initialized;
static bool thread_started;
static bool stopping;
static bool paused;
static float volume = 0.50f;


// WAV information 
static unsigned int sample_rate;
static unsigned int channels;
static uint32_t data_size;
static long data_start;
static uint32_t data_remaining;


// Read little-endian WAV fields. 
static uint16_t read_u16_le(const unsigned char *p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}


static uint32_t read_u32_le(const unsigned char *p)
{
    return (uint32_t)p[0]
         | ((uint32_t)p[1] << 8)
         | ((uint32_t)p[2] << 16)
         | ((uint32_t)p[3] << 24);
}


// Read and validate the WAV header. 
static bool read_wav_header(void)
{
    unsigned char header[12];

    if (fread(header, 1, sizeof(header), wav_file) != sizeof(header))
        return false;

    if (memcmp(header, "RIFF", 4) != 0 ||
        memcmp(header + 8, "WAVE", 4) != 0) {

        fprintf(stderr, "Audio: not a RIFF/WAVE file\n");
        return false;
    }

    bool found_fmt = false;
    bool found_data = false;

    uint16_t format = 0;
    uint16_t bits_per_sample = 0;

    while (!found_data) {

        unsigned char chunk[8];

        if (fread(chunk, 1, sizeof(chunk), wav_file) != sizeof(chunk))
            break;

        uint32_t chunk_size = read_u32_le(chunk + 4);

        long chunk_start = ftell(wav_file);

        if (chunk_start < 0)
            return false;

        if (memcmp(chunk, "fmt ", 4) == 0) {

            unsigned char fmt[16];

            if (chunk_size < sizeof(fmt) ||
                fread(fmt, 1, sizeof(fmt), wav_file) != sizeof(fmt)) {

                fprintf(stderr, "Audio: invalid WAV format chunk\n");
                return false;
            }

            format = read_u16_le(fmt);
            channels = read_u16_le(fmt + 2);
            sample_rate = read_u32_le(fmt + 4);
            bits_per_sample = read_u16_le(fmt + 14);

            found_fmt = true;

        } else if (memcmp(chunk, "data", 4) == 0) {

            data_start = chunk_start;
            data_size = chunk_size;
            found_data = true;
            break;
        }

        // RIFF chunks are padded to an even byte boundary. 
        long next = chunk_start
                  + (long)chunk_size
                  + (long)(chunk_size & 1U);

        if (fseek(wav_file, next, SEEK_SET) != 0)
            return false;
    }

    if (!found_fmt || !found_data ||
        format != 1 ||
        bits_per_sample != 16 ||
        (channels != 1 && channels != 2) ||
        sample_rate == 0) {

        fprintf(stderr,
                "Audio: expected PCM WAV, 16-bit, mono/stereo\n");

        return false;
    }

    size_t bytes_per_frame = channels * sizeof(int16_t);

    if (data_size < bytes_per_frame ||
        data_size % bytes_per_frame != 0) {

        fprintf(stderr, "Audio: invalid WAV data chunk size\n");
        return false;
    }

    data_remaining = data_size;

    return fseek(wav_file, data_start, SEEK_SET) == 0;
}

// Wait for playback to resume if paused
static bool wait_if_paused(bool *pcm_suspended)
{
    pthread_mutex_lock(&audio_mutex);

    while (paused && !stopping) {

        pthread_mutex_unlock(&audio_mutex);

        if (!*pcm_suspended) {

            (void)snd_pcm_drop(pcm_handle);
            *pcm_suspended = true;
        }

        pthread_mutex_lock(&audio_mutex);

        while (paused && !stopping)
            pthread_cond_wait(&audio_cond, &audio_mutex);
    }

    bool should_stop = stopping;

    pthread_mutex_unlock(&audio_mutex);

    if (should_stop)
        return false;

    if (*pcm_suspended) {

        int result = snd_pcm_prepare(pcm_handle);

        if (result < 0) {

            fprintf(stderr,
                    "Audio: failed to resume playback: %s\n",
                    snd_strerror(result));

            return false;
        }

        *pcm_suspended = false;
    }

    return true;
}


// Playback thread. 
static void *playback_thread_func(void *arg)
{
    (void)arg;

    int16_t input[FRAMES_PER_BUFFER * 2];
    int16_t output[FRAMES_PER_BUFFER * 2];

    bool pcm_suspended = false;

    for (;;) {

        if (!wait_if_paused(&pcm_suspended))
            break;

        size_t bytes_per_frame = channels * sizeof(int16_t);

        size_t max_bytes =
            FRAMES_PER_BUFFER * bytes_per_frame;

        size_t frames_to_read = FRAMES_PER_BUFFER;

        if (data_remaining < max_bytes) {

            max_bytes = data_remaining;
            frames_to_read = max_bytes / bytes_per_frame;
        }

        // Restart the WAV file when its data chunk ends.
        if (frames_to_read == 0) {

            if (fseek(wav_file, data_start, SEEK_SET) != 0) {
                fprintf(stderr, "Audio: failed to restart WAV file\n");
                break;
            }

            clearerr(wav_file);
            data_remaining = data_size;

            continue;
        }

        size_t bytes_read = fread(input, 1, max_bytes, wav_file);

        if (bytes_read == 0) {

            if (ferror(wav_file)) {
                perror("Audio: reading WAV file failed");
                break;
            }

            if (fseek(wav_file, data_start, SEEK_SET) != 0)
                break;

            clearerr(wav_file);
            data_remaining = data_size;

            continue;
        }

        if (bytes_read % bytes_per_frame != 0) {

            fprintf(stderr, "Audio: incomplete WAV audio frame\n");
            break;
        }

        data_remaining -= (uint32_t)bytes_read;

        size_t frames_read = bytes_read / bytes_per_frame;

        // Apply software volume control. 
        pthread_mutex_lock(&audio_mutex);
        float current_volume = volume;
        pthread_mutex_unlock(&audio_mutex);

        for (size_t i = 0; i < frames_read * channels; i++) {

            long scaled =
                (long)((float)input[i] * current_volume);

            if (scaled > INT16_MAX)
                scaled = INT16_MAX;

            if (scaled < INT16_MIN)
                scaled = INT16_MIN;

            output[i] = (int16_t)scaled;
        }

        snd_pcm_sframes_t written = 0;

        while ((size_t)written < frames_read) {

            if (!wait_if_paused(&pcm_suspended))
                goto playback_exit;

            snd_pcm_sframes_t result = snd_pcm_writei(
                pcm_handle,
                output + (size_t)written * channels,
                frames_read - (size_t)written
            );

            if (result == -EINTR)
                continue;

            if (result < 0) {

                result = snd_pcm_recover(
                    pcm_handle, (int)result, 1
                );

                if (result < 0) {

                    fprintf(stderr,
                            "Audio: ALSA playback failed: %s\n",
                            snd_strerror((int)result));

                    goto playback_exit;
                }

                continue;
            }

            if (result == 0) {

                fprintf(stderr,
                        "Audio: ALSA returned zero written frames\n");

                goto playback_exit;
            }

            written += result;
        }
    }

playback_exit:

    pthread_mutex_lock(&audio_mutex);
    stopping = true;
    pthread_cond_broadcast(&audio_cond);
    pthread_mutex_unlock(&audio_mutex);

    return NULL;
}


// Initialize the audio system. 
bool audio_init(const char *wav_path)
{
    if (initialized || wav_path == NULL)
        return false;

    wav_file = fopen(wav_path, "rb");

    if (wav_file == NULL) {
        perror("Audio: opening WAV file failed");
        return false;
    }

    if (!read_wav_header()) {

        fprintf(stderr, "Audio: could not parse WAV file\n");

        fclose(wav_file);
        wav_file = NULL;

        return false;
    }

    int result = snd_pcm_open(
        &pcm_handle,
        ALSA_DEVICE,
        SND_PCM_STREAM_PLAYBACK,
        0
    );

    if (result < 0) {

        fprintf(stderr,
                "Audio: opening ALSA device failed: %s\n",
                snd_strerror(result));

        fclose(wav_file);
        wav_file = NULL;

        return false;
    }

     // Configure ALSA for fixed 48 kHz output.
     // Software resampling is enabled for other WAV sample rates.
    
    result = snd_pcm_set_params(
        pcm_handle,
        SND_PCM_FORMAT_S16_LE,
        SND_PCM_ACCESS_RW_INTERLEAVED,
        channels,
        OUTPUT_SAMPLE_RATE,
        1,
        500000
    );

    if (result < 0) {

        fprintf(stderr,
                "Audio: configuring ALSA failed: %s\n",
                snd_strerror(result));

        snd_pcm_close(pcm_handle);
        pcm_handle = NULL;

        fclose(wav_file);
        wav_file = NULL;

        return false;
    }

    pthread_mutex_lock(&audio_mutex);

    stopping = false;
    paused = false;
    volume = 0.50f;

    pthread_mutex_unlock(&audio_mutex);

    initialized = true;

    return true;
}


// Start audio playback. 
bool audio_play(void)
{
    if (!initialized || thread_started)
        return false;

    pthread_mutex_lock(&audio_mutex);

    stopping = false;
    paused = false;

    pthread_mutex_unlock(&audio_mutex);

    int result = pthread_create(
        &playback_thread,
        NULL,
        playback_thread_func,
        NULL
    );

    if (result != 0) {

        fprintf(stderr,
                "Audio: pthread_create failed: %d\n",
                result);

        return false;
    }

    thread_started = true;

    return true;
}


// Pause audio playback.
void audio_pause(void)
{
    if (!initialized || !thread_started)
        return;

    pthread_mutex_lock(&audio_mutex);

    paused = true;
    pthread_cond_broadcast(&audio_cond);

    pthread_mutex_unlock(&audio_mutex);
}


// Resume audio playback. 
void audio_resume(void)
{
    if (!initialized || !thread_started)
        return;

    pthread_mutex_lock(&audio_mutex);

    paused = false;
    pthread_cond_broadcast(&audio_cond);

    pthread_mutex_unlock(&audio_mutex);
}


// Adjust software volume. 
void audio_set_volume(float new_volume)
{
    if (!isfinite(new_volume))
        return;

    if (new_volume < 0.0f)
        new_volume = 0.0f;

    if (new_volume > 1.0f)
        new_volume = 1.0f;

    pthread_mutex_lock(&audio_mutex);

    volume = new_volume;

    pthread_mutex_unlock(&audio_mutex);
}


// Stop playback and release resources. 
void audio_cleanup(void)
{
    if (!initialized)
        return;

    pthread_mutex_lock(&audio_mutex);

    stopping = true;
    paused = false;

    pthread_cond_broadcast(&audio_cond);

    pthread_mutex_unlock(&audio_mutex);

    // Wait for the playback thread before closing ALSA.
    // The playback thread owns all PCM operations.
    if (thread_started) {

        pthread_join(playback_thread, NULL);
        thread_started = false;
    }

    if (pcm_handle != NULL) {

        (void)snd_pcm_drop(pcm_handle);
        (void)snd_pcm_close(pcm_handle);

        pcm_handle = NULL;
    }

    if (wav_file != NULL) {

        fclose(wav_file);
        wav_file = NULL;
    }

    initialized = false;
}





#ifndef _AUDIO_H
#define _AUDIO_H

#include <stdbool.h>

bool audio_init(const char *wav_path);
bool audio_play(void);
void audio_pause(void);
void audio_resume(void);
void audio_set_volume(float volume);
void audio_cleanup(void);

#endif




#ifndef __AUDIO_H
#define __AUDIO_H

#include <ao/ao.h>

#define M_PI 3.14159265358979323846264338327

typedef struct audio {
    ao_sample_format format;
    ao_device *device;
} audio_t;

audio_t *audio_init();
void audio_beep(audio_t *);
void audio_close(audio_t *);

#endif

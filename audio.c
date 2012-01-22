#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <ao/ao.h>

#include "audio.h"

audio_t *audio_init() {
    audio_t *audio = calloc(1, sizeof(audio_t));

    if(audio == NULL) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    // ao - initialize
    ao_initialize();

    // ao - setup default driver
    int default_driver = ao_default_driver_id();

    memset(&audio->format, 0, sizeof(audio->format));
    audio->format.bits = 16;
    audio->format.channels = 2;
    audio->format.rate = 44100;
    audio->format.byte_format = AO_FMT_LITTLE;

    // ao - open driver
    audio->device = ao_open_live(default_driver, &audio->format, NULL);
    if(audio->device == NULL) {
        fprintf(stderr, "error opening device!\n");
        exit(EXIT_FAILURE);
    }

    return audio;
}

static int audio_buf_size(audio_t * audio) {
    return audio->format.bits / 8 * audio->format.channels *
        audio->format.rate;
}

void audio_beep(audio_t * audio) {
    float freq = 440.0;
    int buf_size = audio_buf_size(audio);
    char *buffer = calloc(buf_size, sizeof(char));

    if(buffer == NULL) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    for(int i = 0; i < audio->format.rate; i++) {
        int sample =
            (int) (.75 * 32768.0 *
                   sin(2 * M_PI * freq * ((float) i / audio->format.rate)));

        buffer[4 * i] = buffer[4 * i + 2] = sample & 0xff;
        buffer[4 * i + 1] = buffer[4 * i + 3] = (sample >> 8) & 0xff;
    }

    ao_play(audio->device, buffer, buf_size);
}

void audio_close(audio_t * audio) {
    ao_close(audio->device);
    ao_shutdown();

    free(audio);
}

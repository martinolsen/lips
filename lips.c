#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "audio.h"

static void beep() {
    printf("beep!\n");
    audio_t *audio = audio_init();

    audio_beep(audio);
    audio_close(audio);
}

int main(void) {
    fprintf(stderr, "       LISP  Interactive  Performance  System\n");

    beep();

    exit(EXIT_SUCCESS);
}

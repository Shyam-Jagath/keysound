#include "input.h"
#include "audio.h"
#include "keymap.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

volatile sig_atomic_t keep_running = 1;

void handle_signal(int sig){
    if (sig == SIGINT || sig == SIGTERM){
        keep_running = 0;
    }
}

void on_key_pressed(int keycode){
    KeyGroup group = get_group(keycode);
    audio_play_group(group);
}

int main(int argc, char *argv[]){
    // Register signal handlers for graceful shutdown
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    printf("Starting keysound daemon...\n");

    audio_init();

    audio_load_pack("current");

    printf("Listening for keyboard events...\n");
    input_start(on_key_pressed);

    printf("Shutting down keysound daemon...\n");
    audio_cleanup();

    return 0;
}

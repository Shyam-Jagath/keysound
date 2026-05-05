#include "input.h"
#include "audio.h"
#include "keymap.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>

#define CONFIG_PATH ".config/keysound"
#define SYSTEM_PACKS_PATH "/usr/share/keysound/soundpacks"
#define CURRENT_LINK "current"

volatile sig_atomic_t keep_running = 1;

void handle_signal(int sig){
    if (sig == SIGINT || sig == SIGTERM){
        keep_running = 0;
    }
}

int dir_exists(const char *dir){

    struct stat st;
    if(stat(dir,&st)!= 0){
        return 0;
    }
    if(S_ISDIR(st.st_mode)){
        return 1;
    }
    return 0;
}

void on_key_pressed(int keycode){
    KeyGroup group = get_group(keycode);
    audio_play_group(group);
}

void link_to_any_random_soundpack(const char *config_path , const char *link_path){
    DIR *dir = opendir(config_path);
    if(!dir){
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL){

        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0 || strcmp(entry->d_name, CURRENT_LINK) == 0){
            continue;
        }

        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", config_path, entry->d_name);
        
        if (dir_exists(full_path)){
            printf("no 'default' folder found, falling back to pack '%s'\n", entry->d_name);
            
            if (symlink(full_path, link_path) != 0){
                perror("error creating fallback symlink");
            }
            closedir(dir);
            return; 
        }
    }
    closedir(dir);

    printf("wtf?, the keysound directory is totally empty, please add soundpacks to %s\n", config_path);
}

void ensure_default_pack(){
    const char *home = getenv("HOME");
    if (!home) return;

    char base_path[PATH_MAX];
    char link_path[PATH_MAX];
    char default_pack_path[PATH_MAX];

    snprintf(base_path, sizeof(base_path), "%s/%s", home, CONFIG_PATH);
    snprintf(link_path, sizeof(link_path), "%s/%s", base_path, CURRENT_LINK);
    snprintf(default_pack_path, sizeof(default_pack_path), "%s/default", base_path);

    if (!dir_exists(base_path)) {
        mkdir(base_path, 0755);
    }

    if (access(link_path, F_OK) == 0){
        return; 
    }

    const char *system_default = "/usr/share/keysound/soundpacks/default";
    
    if (dir_exists(system_default)) {
        printf("first run detected: Linking 'current' to the system 'default' soundpack\n");
        if (symlink(system_default, link_path) != 0) {
            perror("error: Failed to create system default symlink");
        }
    } else if (dir_exists(default_pack_path)){
        printf("first run detected: Linking 'current' to the local 'default' soundpack\n");
        if (symlink(default_pack_path, link_path) != 0) {
            perror("error: Failed to create local default symlink");
        }
    } else {
        link_to_any_random_soundpack(SYSTEM_PACKS_PATH, link_path);
    }
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
    ensure_default_pack();

    audio_init();

    audio_load_pack("current");

    printf("Listening for keyboard events...\n");
    input_start(on_key_pressed);

    printf("Shutting down keysound daemon...\n");
    audio_cleanup();

    return 0;
}

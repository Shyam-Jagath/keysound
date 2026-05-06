#include "keymap.h"
#include <fcntl.h>
#include <unistd.h>
#define MINIAUDIO_IMPLEMENTATION
#include "audio.h"
#include <miniaudio/miniaudio.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include "log.h"

#define MAX_SOUNDS_PER_GROUP 4

typedef struct{
    ma_sound sounds[MAX_SOUNDS_PER_GROUP];
    int count;
}SoundGroup;


static SoundGroup groups[GROUP_COUNT];

static ma_engine engine;

void audio_init(){
    if(ma_engine_init(NULL, &engine) != MA_SUCCESS){
        LOG_ERR("failed to audio init");
        exit(1);
    }
    srand(time(NULL));
}

static void add_sound(KeyGroup group, const char *filepath){
    SoundGroup *g = &groups[group];

    const char *group_name = group_to_string(group);

    if(g->count >= MAX_SOUNDS_PER_GROUP){
        LOG_ERR("Too many sounds in the group %s", group_name);
        return;
    }

    ma_result result = ma_sound_init_from_file(&engine,filepath,0,NULL,NULL,&g->sounds[g->count]);
    if(result != MA_SUCCESS){
        LOG_ERR("Unable to load the audio at %s", filepath);
        return;
    }

    g->count++;

    LOG_DEBUG("Audio at %s loaded successfully to group %s", filepath, group_name);
}

void load_audio_from_directory(const char *base_path){
    for (int i = 0; i < GROUP_COUNT; i++) {
        KeyGroup g = (KeyGroup)i;
        const char *group_name = group_to_string(g);
        
        char group_path[512];
        snprintf(group_path, sizeof(group_path), "%s/%s", base_path, group_name);
        
        DIR *dir = opendir(group_path);
        if (!dir) continue;
        
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
            
            char fullpath[1024];
            snprintf(fullpath, sizeof(fullpath), "%s/%s", group_path, entry->d_name);
            add_sound(g, fullpath);
        }
        closedir(dir);
    }
}

void audio_load_pack(const char *pack){

    char base_path[512];

    snprintf(base_path, sizeof(base_path),
             "%s/.config/keysound/%s/sounds",
             getenv("HOME"), pack);

    LOG_DEBUG("%s <- base path", base_path);
    load_audio_from_directory(base_path);
}


void audio_play_group(KeyGroup group){

    SoundGroup *g = &groups[group];
    if(g->count == 0) {
        return;
    }
    int index = rand() % g->count;
    ma_sound *s = &g->sounds[index];
    ma_sound_seek_to_pcm_frame(s, 0);
    ma_sound_start(s);
}

void audio_cleanup(){

    for(int i=0;i<GROUP_COUNT;i++){
        SoundGroup *g = &groups[i];
        
        for(int j=0;j<g->count;j++) {
            ma_sound_uninit(&g->sounds[j]);
        }
        g->count = 0;
    }

    ma_engine_uninit(&engine);

    LOG_INFO("Audio cleanup done");
}
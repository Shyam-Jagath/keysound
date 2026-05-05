#include "keymap.h"
#include <fcntl.h>
#include <unistd.h>
#define MINIAUDIO_IMPLEMENTATION
#include "audio.h"
#include <miniaudio/miniaudio.h>
#include <stdio.h>
#include <stdlib.h>
#include <cjson/cJSON.h>

#define MAX_SOUNDS_PER_GROUP 4

typedef struct{
    ma_sound sounds[MAX_SOUNDS_PER_GROUP];
    int count;
}SoundGroup;


static SoundGroup groups[GROUP_COUNT];

static ma_engine engine;

void audio_init(){
    if(ma_engine_init(NULL, &engine) != MA_SUCCESS){
        printf("\nfailed to audio init\n");
        exit(1);
    }
    srand(time(NULL));
}

static void add_sound(KeyGroup group, const char *filepath){
    SoundGroup *g = &groups[group];

    const char *group_name = group_to_string(group);

    if(g->count >= MAX_SOUNDS_PER_GROUP){
        printf("Too many sounds in the group %s",group_name);
        return;
    }

    ma_result result = ma_sound_init_from_file(&engine,filepath,0,NULL,NULL,&g->sounds[g->count]);
    if(result != MA_SUCCESS){
        printf("Unable to load the audio at %s",filepath);
        return;
    }

    g->count++;

    printf("\nAudio at %s loaded successfully to group %s\n",filepath,group_name);
}

void load_audio_from_json(const char *config_path , const char *base_path){
    FILE *f = fopen(config_path,"r");

    if(!f){
        printf("unable to open the config file");
        return;
    }

    fseek(f,0,SEEK_END);
    long size = ftell(f);
    rewind(f);

    char *data = malloc(size+1);
    data[size] = '\0';
    fread(data,1,size, f);
    fclose(f);

    cJSON *json = cJSON_Parse(data);
    
    if(!json){
        printf("it is not json file, maybe corrupted ?");
        free(data);
        return;
    }

    cJSON *group = NULL;
    cJSON_ArrayForEach(group, json){
        KeyGroup g = group_from_string(group->string);

        if(!cJSON_IsArray(group)){
            continue;
        }

        int count = cJSON_GetArraySize(group);

        for (int i = 0; i < count; i++){
            cJSON *item = cJSON_GetArrayItem(group, i);

            if(!cJSON_IsString(item)){
                continue;
            }

            char fullpath[512];
            snprintf(fullpath, sizeof(fullpath),"%s/%s", base_path, item->valuestring);
            add_sound(g, fullpath);
        }
    }

    cJSON_Delete(json);
    free(data);
}

void audio_load_pack(const char *pack){

    char config_path[512];
    char base_path[512];

    snprintf(config_path, sizeof(config_path),
             "%s/.config/keysound/%s/config.json",
             getenv("HOME"), pack);

    snprintf(base_path, sizeof(base_path),
             "%s/.config/keysound/%s/sounds",
             getenv("HOME"), pack);

    printf("\n%s <- config path and %s <- base path\n",config_path,base_path);
    load_audio_from_json(config_path, base_path);
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

    printf("Audio cleanup done\n");
}
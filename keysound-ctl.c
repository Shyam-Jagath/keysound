#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>

#define CONFIG_PATH ".config/keysound"
#define CURRENT_LINK "current"



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

int set_pack(char* pack){
    const char *home = getenv("HOME");
    if(!home){
        printf("unable to retrive HOME\n");
        return 1;
    }

    char base[PATH_MAX];
    char target[PATH_MAX];
    char link[PATH_MAX];

    snprintf(base, sizeof(base),"%s/%s",home,CONFIG_PATH); // ~/.config/keysound
    snprintf(target, sizeof(target), "%s/%s",base,pack); // ~/.config/keysound/<PACK_NAME>
    snprintf(link, sizeof(link), "%s/%s",base,CURRENT_LINK); // ~/.config/keysound/current

    if(!dir_exists(target)){
        printf("Pack '%s' not found\n", pack);
        return 1;
    }

    unlink(link);

    if(symlink(target, link) == 0){
        printf("sound Pack changed to %s\n",pack);

        printf("restarting systemd service....\n");
        int status = system("systemctl --user restart keysound.service");
        if(status == 0){
            printf("sound Pack %s is successfully applied\n",pack);
        }
        else{
            printf("unable to restart the service\n");
        }
        return 0;
    }
    else{
        printf("unable to create symlink\n");
        return 1;
    }
}


void list(const char* base_path){
    DIR *dir = opendir(base_path);
    if(!dir){
        printf("error opening directory '%s'\n", base_path);
        return;
    }

    struct dirent *entry;
    char *packs[256]; 
    int count = 0;

    while((entry = readdir(dir)) != NULL){
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name,"..") == 0 || strcmp(entry->d_name,CURRENT_LINK) ==0){
            continue;
        }
        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s",base_path,entry->d_name);
        
        if(dir_exists(full_path)){
            if(count < 256){
                packs[count] = strdup(entry->d_name);
                count++;
            }
        }
    }
    closedir(dir);

    if(count == 0){
        printf("why there are no soundpacks found in '%s',please add some soundpacks\n", base_path);
        return;
    }

    printf("\nAvailable Soundpacks:\n");
    printf("========================================\n");
    for(int i = 0;i < count;i++){
        printf(" [%d] %s\n", i + 1, packs[i]);
    }
    printf("========================================\n");

    int choice;
    printf("select a soundpack number: ");
    
    if(scanf("%d", &choice) == 1 && choice >= 1 && choice <= count){
        set_pack(packs[choice - 1]);
    } 
    else{
        printf("invalid selection. just like how you selected your crush. Bye boi\n");
    }

    for(int i = 0;i < count;i++){
        free(packs[i]);
    }
}

int main(int argc,char* argv[]){
    const char *home = getenv("HOME");
    if(!home){
        printf("unable to retrieve HOME directory.\n");
        return 1;
    }

    char base_path[PATH_MAX];
    snprintf(base_path, sizeof(base_path), "%s/%s", home, CONFIG_PATH);

    list(base_path);

    return 0;
}


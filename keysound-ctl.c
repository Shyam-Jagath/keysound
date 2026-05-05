#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>

#define CONFIG_PATH ".config/keysound"
#define SYSTEM_PACKS_PATH "/usr/share/keysound/soundpacks"
#define CURRENT_LINK "current"

struct PackInfo {
    char name[256];
    char path[PATH_MAX];
};

int dir_exists(const char *dir){
    struct stat st;
    if(stat(dir,&st)!= 0) return 0;
    if(S_ISDIR(st.st_mode)) return 1;
    return 0;
}

int set_pack(const char* target_path, const char* pack_name){
    const char *home = getenv("HOME");
    if(!home){
        printf("unable to retrieve HOME directory.\n");
        return 1;
    }

    char base[PATH_MAX];
    char link[PATH_MAX];

    snprintf(base, sizeof(base),"%s/%s",home,CONFIG_PATH); // ~/.config/keysound
    snprintf(link, sizeof(link), "%s/%s",base,CURRENT_LINK); // ~/.config/keysound/current

    // Automatically create ~/.config/keysound if it doesn't exist
    if(!dir_exists(base)){
        mkdir(base, 0755);
    }

    if(!dir_exists(target_path)){
        printf("Pack path '%s' not found\n", target_path);
        return 1;
    }

    unlink(link);

    if(symlink(target_path, link) == 0){
        printf("Sound pack changed to %s\n", pack_name);
        printf("Restarting systemd service....\n");
        int status = system("systemctl --user restart keysound.service");
        if(status == 0){
            printf("sound pack %s is successfully applied\n", pack_name);
        } else {
            printf("unable to restart the service\n");
        }
        return 0;
    } else {
        printf("unable to create symlink\n");
        return 1;
    }
}

void load_packs_from_dir(const char* dir_path, struct PackInfo *packs, int *count) {
    DIR *dir = opendir(dir_path);
    if(!dir) return;

    struct dirent *entry;
    while((entry = readdir(dir)) != NULL){
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name,"..") == 0 || strcmp(entry->d_name,CURRENT_LINK) == 0){
            continue;
        }
        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
        
        if(dir_exists(full_path)){
            if(*count < 256){
                int exists = 0;
                for (int i = 0; i < *count; i++) {
                    if (strcmp(packs[i].name, entry->d_name) == 0) {
                        exists = 1;
                        break;
                    }
                }
                if (!exists) {
                    strncpy(packs[*count].name, entry->d_name, sizeof(packs[*count].name) - 1);
                    strncpy(packs[*count].path, full_path, sizeof(packs[*count].path) - 1);

                    packs[*count].name[sizeof(packs[*count].name) - 1] = '\0';
                    packs[*count].path[sizeof(packs[*count].path) - 1] = '\0';
                    (*count)++;
                }
            }
        }
    }
    closedir(dir);
}

void list(const char* user_path){
    struct PackInfo packs[256];
    int count = 0;

    load_packs_from_dir(user_path, packs, &count);
    
    load_packs_from_dir(SYSTEM_PACKS_PATH, packs, &count);

    if(count == 0){
        printf("why there are no soundpacks found, Please add some to %s or %s\n", user_path, SYSTEM_PACKS_PATH);
        return;
    }

    printf("\nAvailable Soundpacks:\n");
    printf("========================================\n");
    for(int i = 0; i < count; i++){
        printf(" [%d] %s\n", i + 1, packs[i].name);
    }
    printf("========================================\n");

    int choice;
    printf("select a soundpack number: ");
    
    if(scanf("%d", &choice) == 1 && choice >= 1 && choice <= count){
        set_pack(packs[choice - 1].path, packs[choice - 1].name);
    } 
    else{
        printf("invalid selection. just like how you selected your crush. Bye boi\n");
    }
}

int main(int argc,char* argv[]){
    const char *home = getenv("HOME");
    if(!home){
        printf("unable to retrieve HOME directory.\n");
        return 1;
    }

    char user_path[PATH_MAX];
    snprintf(user_path, sizeof(user_path), "%s/%s", home, CONFIG_PATH);

    list(user_path);

    return 0;
}


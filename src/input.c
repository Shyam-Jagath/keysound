#include <libevdev/libevdev.h>
#include <libudev.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <poll.h>
#include "input.h"
#define MAX_DEVICES 32

struct device {
    int fd;
    struct libevdev *evdev;
    char path[256];
};

struct device devices[MAX_DEVICES];
int num_devices=0;

static int is_keyboard(struct libevdev *dev){
    return libevdev_has_event_type(dev, EV_KEY);
}

static void add_device(const char *path){
    if(num_devices >= MAX_DEVICES){
        return;
    } 

    int fd=open(path, O_RDONLY | O_NONBLOCK);
    if(fd < 0){
        return;
    }

    struct libevdev *dev=NULL;
    if(libevdev_new_from_fd(fd, &dev) < 0){
        close(fd);
        return;
    }

    if(!is_keyboard(dev)){
        libevdev_free(dev);
        close(fd);
        return;
    }

    devices[num_devices].fd = fd;
    devices[num_devices].evdev = dev;
    strncpy(devices[num_devices].path, path, sizeof(devices[num_devices].path));

    //this print statement is just for testing, later i'll add this into logs
    printf("added: %s %s\n", path, libevdev_get_name(dev));
    num_devices++;
}

static void remove_device(const char *path){
    for(int i=0;i<num_devices;i++){
        
     //this print statement is just for testing, later i'll add this into logs   
        if(strcmp(devices[i].path, path) == 0){
            printf("removed: %s\n", path);

            libevdev_free(devices[i].evdev);
            close(devices[i].fd);

            devices[i]=devices[num_devices-1];
            num_devices--;
            return;
        }
    }
}

static void scan_existing_devices(){
    struct udev *udev = udev_new();
    struct udev_enumerate *enumerate=udev_enumerate_new(udev);

    udev_enumerate_add_match_subsystem(enumerate, "input");
    udev_enumerate_scan_devices(enumerate);

    struct udev_list_entry *devices_list=udev_enumerate_get_list_entry(enumerate);
    struct udev_list_entry *entry;

    udev_list_entry_foreach(entry, devices_list){
        const char *path = udev_list_entry_get_name(entry);
        struct udev_device *dev = udev_device_new_from_syspath(udev, path);

        const char *devnode = udev_device_get_devnode(dev);
        const char *prop = udev_device_get_property_value(dev, "ID_INPUT_KEYBOARD");

        if(devnode && prop && strcmp(prop, "1") == 0){
            add_device(devnode);
        }

        udev_device_unref(dev);
    }

    udev_enumerate_unref(enumerate);
    udev_unref(udev);
}

void input_start(void (*callback)(int)){
    struct udev *udev = udev_new();

    struct udev_monitor *mon = udev_monitor_new_from_netlink(udev, "udev");
    udev_monitor_filter_add_match_subsystem_devtype(mon, "input", NULL);
    udev_monitor_enable_receiving(mon);

    int udev_fd = udev_monitor_get_fd(mon);

    scan_existing_devices();

    if (num_devices == 0) {
        fprintf(stderr, "WARNING: No input devices found! Are you sure you added your user to the 'input' group and logged back in?\n");
    }

    while(keep_running){ // gracefully exit on signal
        struct pollfd pfds[MAX_DEVICES+1];

        pfds[0].fd=udev_fd;
        pfds[0].events=POLLIN;

        for (int i=0;i<num_devices;i++){
            pfds[i+1].fd=devices[i].fd;
            pfds[i+1].events=POLLIN;
        }

        int ret = poll(pfds, num_devices + 1, -1);

        if(ret < 0) continue;

        if(pfds[0].revents & POLLIN){
            struct udev_device *dev = udev_monitor_receive_device(mon);

            const char *action = udev_device_get_action(dev);
            const char *devnode = udev_device_get_devnode(dev);
            const char *prop = udev_device_get_property_value(dev, "ID_INPUT_KEYBOARD");

            if(devnode && prop && strcmp(prop, "1") == 0){
                if(strcmp(action, "add") == 0){
                    add_device(devnode);
                } else if(strcmp(action, "remove") == 0){
                    remove_device(devnode);
                }
            }

            udev_device_unref(dev);
        }

        for (int i=0; i < num_devices; i++){
            if(pfds[i+1].revents & POLLIN){
                struct input_event ev;

                while(libevdev_next_event(devices[i].evdev,LIBEVDEV_READ_FLAG_NORMAL, &ev) == 0){

                    if(ev.type == EV_KEY && ev.value == 1){
                        //this print statement is just for testing, later i'll add logs
                        printf("key pressed: %d %s\n",ev.code,libevdev_event_code_get_name(EV_KEY, ev.code));
                        callback(ev.code);
                    }
                }
            }
        }
    }

}
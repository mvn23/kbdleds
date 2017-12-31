/*
 * File:   main.c
 * Author: milan
 *
 * Created on December 21, 2017, 11:30 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

//Input event structure for interpreting /dev/input/eventX streams
struct input_event {
	struct timeval time;
	unsigned short type;
	unsigned short code;
	unsigned int value;
};

//Find keyboard event device by name
void findKbdPath(char* name, char* path) {
    FILE* fName;
    DIR* inputDir;
    char fContents[strlen(name) + 2];
    unsigned int i;
    struct dirent* entp;
    inputDir = opendir("/sys/class/input");
    while (entp = readdir(inputDir)) {
        if (strncmp(entp->d_name, "event", 5) == 0) {
            strncpy(path, "", 1);
            strncat(path, "/sys/class/input/", 18);
            strncat(path, entp->d_name, 8);
            strncat(path, "/device/name", 13);
            fName = fopen(path, "r");
            if (fName != NULL) {
                strncpy(fContents, "", 1);
                for ( i = 0; i < strlen(name) + 1; i++ ) {
                    int c = fgetc(fName);
                    if ( c == 10 || feof(fName)) {
                        break;
                    }
                    fContents[i] = c;
                    fContents[i+1] = 0;
                }
                fclose(fName);
                if ( strncmp(fContents, name, strlen(name) + 1) == 0 ) {
                    strncpy(path, "/dev/input/", 12); 
                    strncat(path, entp->d_name, 8);
                    break;
                }
            }
        }
    }
    closedir(inputDir);
    return;
}


/*
 *
 */
int main(int argc, char** argv) {
    char kbdName[] = "AT Translated Set 2 keyboard";
    char ledPath[] = "/sys/class/leds/asus::kbd_backlight/";
    char kbdPath[PATH_MAX];
    int timeout = 15;
    FILE* kbdEvent;
    size_t eventsize = sizeof (struct input_event);
    struct input_event* evntp = malloc(eventsize);
    fd_set rfds;
    struct timeval timer;
    int retval;
    
    findKbdPath(kbdName, kbdPath);
    kbdEvent = fopen(kbdPath, "rb");
    if ( kbdEvent != NULL ) {
        
        int fd = fileno(kbdEvent);
        
        while ( 1 ) {
            FD_ZERO(&rfds);
            FD_SET(fd, &rfds);
            
            timer.tv_sec = timeout;
            timer.tv_usec = 0;
            
            retval =  select(fd+1, &rfds, NULL, NULL, &timer);

            if (retval == -1) {
                perror("select()");
            } else if (retval) {
                printf("Data should be available now.\n");
                if ( fread(evntp, eventsize, 1, kbdEvent) == 1 ) {
                    //TODO: restore keyboard brightness if necessary.
                    //TODO: catch XF86KbdBrightness* keys and adjust brightness accordingly.
                } else {
                    printf("Error reading from stream.");
                    break;
                }
            } else {
                printf("No data within %i seconds.\n", timeout);
                //TODO: store brightness value if > 0 and turn keyboard lights off.
            }            
        }
    } else {
        printf("Error opening keyboard stream.");
    }
    fclose(kbdEvent);
    free(evntp);
}

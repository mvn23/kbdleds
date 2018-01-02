/*
 * File:   main.c
 * Author: milan
 *
 * Created on December 21, 2017, 11:30 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/limits.h>
#include <math.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>


/**
 * findKbdPath() find keyboard event device @path by @name
 * @param name Pointer to char[] holding the keyboard name.
 * @param path Pointer to char[] to hold the device path.
 */
void findKbdPath(char* name, char* path) {
    FILE *fName;
    DIR *inputDir;
    char fContents[strlen(name) + 2];
    unsigned int i;
    struct dirent *entp;
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

/**
 * storeBrt() store brightness value of led at @control into @value.
 * @param value Pointer to an int to hold the value read from @control.
 * @param control Pointer to an open FILE stream to the brightness control.
 */
void storeBrt(int* value, FILE* control) {
    fseek(control, 0, SEEK_SET);
    fscanf(control, "%u", value);
}

/**
 * getBrt() get actual brightness of @control
 * @param control Pointer to an open FILE stream to the brightness control.
 * @return int Actual brightness of @control.
 */
int getBrt(FILE* control) {
    int value;
    storeBrt(&value, control);
    return(value);
}

/**
 * setBrt() set the brightness of led at @control to @value, between 0 and @max.
 * @param control Pointer to an open r/w FILE stream to the brightness control.
 * @param value Desired new brightness value.
 * @param max Maximum brightness.
 * @return 0 on successful change, 1 if no change, -1 on error.
 */
int setBrt(FILE* control, int value, int max) {
    int cur = getBrt(control);
    int ret;
    if ( value == cur || (value < 0 && cur == 0) || (value > max && cur == max) ) {
        return(1);        
    }
    value = ( value < 0 ) ? 0 : (value < max) ? value : max;
    fseek(control, 0, SEEK_SET);
    ret = fprintf(control, "%d\n", value);
    fflush(control);
    if ( ret < 0 ) {
        return(-1);
    }
    return(0);
}


/**
 * main() main function.
 * @param argc
 * @param argv
 * @return 
 */
int main(int argc, char** argv) {
    char kbdName[] = "AT Translated Set 2 keyboard";
    char ledPath[] = "/sys/class/leds/asus::kbd_backlight/";
    char maxPath[strlen(ledPath)+15];
    char curPath[strlen(ledPath)+11];
    char kbdPath[PATH_MAX];
    int timeout = 15;
    int curBrt = 0;
    int maxBrt = 0;
    FILE *ledBrt;
    FILE *kbdEvent;
    size_t eventsize = sizeof (struct input_event);
    struct input_event *evntp = malloc(eventsize);
    fd_set rfds;
    struct timeval timer;
    int retval;
    
    strncpy(maxPath, ledPath, strlen(ledPath)+1);
    strncpy(curPath, ledPath, strlen(ledPath)+1);
    
    //Determine max brightness
    ledBrt = fopen(strncat(maxPath, "max_brightness", 15), "r");
    if ( ledBrt != NULL ) {
        storeBrt(&maxBrt, ledBrt);
    } else {
        perror("Could not determine keyboard's maximum brightness");
        exit(1);
    }
    
    //Determine current brightness
    ledBrt = freopen(strncat(curPath, "brightness", 11), "r+", ledBrt);
    if ( ledBrt != NULL ) {
        storeBrt(&curBrt, ledBrt);
    } else {
        perror("Could not determine keyboard's current brightness");
        exit(1);
    }
    
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
                //printf("Data should be available now.\n");
                if ( fread(evntp, eventsize, 1, kbdEvent) == 1 ) {
                    //TODO: restore keyboard brightness if necessary.
                    if ( evntp->type == EV_KEY && evntp->value != 0 ) {
                        setBrt(ledBrt, curBrt, maxBrt);
                        //printf("timeval - tv_sec:\t%ld\n", evntp->time.tv_sec);
                        //printf("timeval - tv_usec:\t%ld\n", evntp->time.tv_usec);
                        //printf("type:\t%hu\n", evntp->type);
                        //printf("code:\t%hu\n", evntp->code);
                        //printf("value:\t%u\n", evntp->value);
                    }
                    //TODO: catch XF86KbdBrightness* keys and adjust brightness accordingly.
                    //      unfortunately these keys are sent from a different input device...;
                } else {
                    perror("Error reading from stream.");
                    break;
                }
            } else {
                //printf("No data within %i seconds.\n", timeout);
                //TODO: store brightness value if > 0 and turn keyboard lights off.
                if ( getBrt(ledBrt) > 0 ) {
                    storeBrt(&curBrt, ledBrt);
                }
                setBrt(ledBrt, 0, maxBrt);
            }
        }
    } else {
        perror("Error opening keyboard stream.");
    }
    fclose(kbdEvent);
    fclose(ledBrt);
    free(evntp);
}

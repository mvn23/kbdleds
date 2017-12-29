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
#include <dirent.h>

//Find keyboard event device by name
void findKbdPath(char* name, char* path) {
    FILE *fName;
    DIR *inputDir;
    char fContents[strlen(name) + 1];
    int i;
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
                        i--;
                        break;
                    }
                   fContents[i] = c;
                }
                fContents[i+1] = 0;
                fclose(fName);
                if ( strncmp(fContents, name, strlen(name) + 1) == 0 ) {
                    strncpy(path, "/dev/input/", 12); 
                    strncat(path, entp->d_name, 8);
                    return;
                }
            }
        }
    }
    return;
}

/*
 *
 */
int main(int argc, char** argv) {
    char kbdName[] = "AT Translated Set 2 keyboard";
    char ledPath[] = "/sys/class/leds/asus::kbd_backlight/";
    char kbdPath[PATH_MAX];
    findKbdPath(kbdName, kbdPath);
    printf("Keyboard path: %s", kbdPath);
    return (EXIT_SUCCESS);
}

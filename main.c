/* 
 * File:   main.c
 * Author: milan
 *
 * Created on December 21, 2017, 11:30 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>

/*
 * 
 */
int main(int argc, char** argv) {
    char kbdName[] = "AT Translated Set 2 keyboard";
    char ledPath[] = "/sys/class/leds/asus::kbd_backlight/";
    char kbdPath[] = findKbdPath(kbdName);
    
    return (EXIT_SUCCESS);
}

char *findKbdPath(char* name) {
    DIR *inputDir;
    struct dirent *entp;
    inputDir = opendir(name);
    
}

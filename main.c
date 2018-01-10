/*
 * Copyright (C) 2018 Milan van Nugteren <milan at network23.nl>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* 
 * File:   main.c
 * Author: Milan van Nugteren <milan at network23.nl>
 *
 * Created on January 10, 2018, 2:19 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/limits.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>

struct config_container {
    char configPath[PATH_MAX];
    char ledPath[PATH_MAX];
    int startval;
    int timeout;
    char use_lid;
};

/**
 * showHelp() should be self explanatory...
 */
void showHelp() {
    printf("Usage: kbdleds [OPTION]...\n");
    printf("Control your keyboard led lights.\n");
    printf("\n");
    printf("\t-c\t--config [FILE]\tLoad config from [FILE]\n");
    printf("\t-h\t--help\t\t\tShow this help message\n");
    printf("\t-l\t--ledpath [PATH]\tSet the base path to your led control\n");
    printf("\t-s\t--startvalue [VALUE]\tSet brightness to [VALUE] on start\n");
    printf("\t\t\t\t\t(-1 to set max brightness, -2 for no change)\n");
    printf("\t-t\t--timeout [VALUE]\tSet the inactivity timeout\n");
    printf("\t-u\t--use-lid\t\tReact to lid open/close events\n");
    exit(0);
}

/**
 * findKbdPaths() find keyboard event device @paths
 * @param paths Pointer to array of char[] to hold the device paths.
 *
 */
void findKbdPaths(int* numKbds, char*** pathptr, struct config_container* config) {
    FILE *fName;
    DIR *inputDir;
    char contents[12];
    char lidsw[] = "Lid Switch\n";
    char path[PATH_MAX];
    struct dirent *entp;
    char** paths = NULL;

    inputDir = opendir("/sys/class/input");
    while (entp = readdir(inputDir)) {
        if (strncmp(entp->d_name, "event", 5) == 0) {
            strncpy(path, "/sys/class/input/", 18);
            strncat(path, entp->d_name, 8);
            strncat(path, "/device/capabilities/key", 25);
            fName = fopen(path, "r");
            if (fName == NULL) {
                perror("Error reading from file");
                exit(1);
            }
            strncpy(contents, "", 1);
            if ( fgets(contents, 12, fName) != NULL ) {
                if ( strncmp(contents, "0\n", 3) != 0 ) {
                    paths = realloc(paths, (*numKbds+1) * sizeof(char*));
                    paths[*numKbds] = malloc(strlen(entp->d_name) + 12);
                    strncpy(paths[*numKbds], "/dev/input/", 12);
                    strncat(paths[*numKbds], entp->d_name, strlen(entp->d_name));
                    ++*numKbds;
                    fclose(fName);
                    continue;
                }
            }
            fclose(fName);

            if ( config->use_lid ) {
                strncpy(path, "/sys/class/input/", 18);
                strncat(path, entp->d_name, 8);
                strncat(path, "/device/name", 13);
                fName = fopen(path, "r");
                if (fName == NULL) {
                    perror("Error reading from file");
                    exit(1);
                }
                strncpy(contents, "", 1);
                if ( fgets(contents, 12, fName) != NULL ) {
                    if ( strncmp(contents, lidsw, 12) == 0 ) {
                        paths = realloc(paths, (*numKbds+1) * sizeof(char*));
                        paths[*numKbds] = malloc(strlen(entp->d_name) + 12);
                        strncpy(paths[*numKbds], "/dev/input/", 12);
                        strncat(paths[*numKbds], entp->d_name, 8);
                        ++*numKbds;
                    }
                }
                fclose(fName);
            }
        }
    }
    *pathptr = paths;
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
 * parseOpts() parse command line options.
 * @param config Pointer to config_container in which the options will be stored.
 * @param argc Number of arguments (as in main()).
 * @param argv Array of arguments (as in main()).
 */
void parseOpts(struct config_container* config, int argc, char** argv) {
    int opt;

    struct option options[] = {
        {"config", required_argument, NULL, 'c'},
        {"help", no_argument, NULL, 'h'},
        {"ledpath", required_argument, NULL, 'l'},
        {"startvalue", required_argument, NULL, 's'},
        {"timeout", required_argument, NULL, 't'},
        {"use-lid", no_argument, NULL, 'u'},
        { NULL, 0, NULL, 0 }
    };

    while ( (opt = getopt_long(argc, argv, "c:hl:s:t:u", options, NULL)) != -1 ) {
        switch ( opt ) {
            case 'c':
                strncpy(config->configPath, optarg, PATH_MAX);
                break;
            case 'h':
                showHelp();
                break;
            case 'l':
                strncpy(config->ledPath, optarg, PATH_MAX);
                break;
            case 's':
                config->startval = (int) *optarg;
                break;
            case 't':
                config->timeout = (int) *optarg;
                break;
            case 'u':
                config->use_lid = 1;
                break;
        }
    }
 }


/**
 * main() main function.
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char** argv) {
    struct config_container config;
    config.configPath[0] = '\0';
    config.ledPath[0] = '\0';
    config.startval = -2;
    config.timeout = 15;
    config.use_lid = 0;
    int curBrt = 0;
    int maxBrt = 0;
    FILE *ledBrt;
    FILE **kbdEvent;
    size_t eventsize = sizeof (struct input_event);
    struct input_event *evntp = malloc(eventsize);
    fd_set rfds;
    int nfds = 1;
    int fd;
    struct timeval timer;
    int retval;
    char** kbdPaths = NULL;
    int numKbds = 0;

    //TODO: read config file

    parseOpts(&config, argc, argv);

    char maxPath[strlen(config.ledPath)+16];
    char curPath[strlen(config.ledPath)+12];

    strncpy(maxPath, config.ledPath, strlen(config.ledPath)+1);
    strncpy(curPath, config.ledPath, strlen(config.ledPath)+1);

    if ( config.ledPath[0] == '\0' ) {
        showHelp();
    }
    //Determine max brightness
    ledBrt = fopen(strncat(maxPath, "/max_brightness", 16), "r");
    if ( ledBrt != NULL ) {
        storeBrt(&maxBrt, ledBrt);
    } else {
        perror("Could not determine keyboard's maximum brightness. Is ledpath set correctly?");
        exit(1);
    }

    //Determine current brightness
    ledBrt = freopen(strncat(curPath, "/brightness", 12), "r+", ledBrt);
    if ( ledBrt != NULL ) {
        storeBrt(&curBrt, ledBrt);
    } else {
        perror("Could not determine keyboard's current brightness");
        exit(1);
    }

    findKbdPaths(&numKbds, &kbdPaths, &config);
    kbdEvent = malloc(numKbds * sizeof(FILE*));

    for ( int i = 0; i < numKbds; ++i ) {
        kbdEvent[i] = fopen(kbdPaths[i], "rb");
        if ( kbdEvent[i] == NULL ) {
            perror("Could not open event interface.");
        } else {
            setvbuf(kbdEvent[i], NULL, _IONBF, 0);
        }
    }
    
    if ( config.startval > -1 ) {
        setBrt(ledBrt, config.startval, maxBrt);
    } else if ( config.startval == -1 ) {
        setBrt(ledBrt, maxBrt, maxBrt);
    }

    while ( 1 ) {
        nfds = 1;
        FD_ZERO(&rfds);
        for ( int i = 0; i < numKbds; ++i ) {
            if ( kbdEvent[i] != NULL ) {
                fd = fileno(kbdEvent[i]);
                FD_SET(fd, &rfds);
                nfds = ( nfds > fd+1 ) ? nfds : (fd+1);
            }
        }

        if ( nfds == 1 ) {
            perror("No event interfaces could be opened.");
            exit(1);
        }

        timer.tv_sec = config.timeout;
        timer.tv_usec = 0;

        retval = select(nfds, &rfds, NULL, NULL, &timer);

        if ( retval == -1 ) {
            perror("Error in select()");
        } else if ( retval ) {
            //Data available
            for ( int i = 0; i < numKbds; ++i ) {
                if ( FD_ISSET(fileno(kbdEvent[i]), &rfds) ) {
                    if ( fread(evntp, eventsize, 1, kbdEvent[i]) == 1 ) {
                        if ( evntp->type == EV_KEY && evntp->value != 0 ) {
                            if ( evntp->code == KEY_KBDILLUMUP ) {
                                ( curBrt < maxBrt ) ? ++curBrt : curBrt;
                            } else if ( evntp->code == KEY_KBDILLUMDOWN ) {
                                ( curBrt > 0 ) ? --curBrt : curBrt;
                            } else if ( evntp->code == KEY_KBDILLUMTOGGLE ) {
                                if ( getBrt(ledBrt) == 0 ) {
                                    curBrt = maxBrt;
                                } else {
                                    curBrt = 0;
                                }
                            }
                            setBrt(ledBrt, curBrt, maxBrt);
                        } else if ( config.use_lid && evntp->type == EV_SW 
                                && evntp->code == SW_LID ) {
                            if ( evntp->value ) { //Lid closed
                                if ( getBrt(ledBrt) > 0 ) {
                                    storeBrt(&curBrt, ledBrt);
                                }
                                setBrt(ledBrt, 0, maxBrt);
                            } else { //Lid opened
                                setBrt(ledBrt, curBrt, maxBrt);
                            }
                        }
                    } else {
                        perror("Error reading from stream.");
                        break;
                    }
                }
            }
        } else {
            //Timeout reached
            if ( getBrt(ledBrt) > 0 ) {
                storeBrt(&curBrt, ledBrt);
            }
            setBrt(ledBrt, 0, maxBrt);
        }
    }

    for ( int i = 0; i < numKbds; ++i ) {
        fclose(kbdEvent[i]);
        free(kbdPaths[i]);
    }
    free(kbdEvent);
    free(kbdPaths);
    fclose(ledBrt);
    free(evntp);
}

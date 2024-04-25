/**
 * @file imgfscmd.c
 * @brief imgFS command line interpreter for imgFS core commands.
 *
 * Image Filesystem Command Line Tool
 *
 * @author Mia Primorac
 */

#include "imgfs.h"
#include "imgfscmd_functions.h"
#include "util.h"   // for _unused

#include <stdlib.h>
#include <string.h>
#include <vips/vips.h>

/*******************************************************************************
 * MAIN
 */

//commands that need to be parsed array !!!
// Command structure
//Typedef for command function pointer
typedef int (*command)(int argc, char *argv[]);

// Struct for command mapping
typedef struct {
    const char *name;
    command command_function;
} command_mapping;


// Array of commands
command_mapping commands[] = {
        {"help",   help},
        {"list",   do_list_cmd},
        {"create", do_create_cmd},
        {"delete", do_delete_cmd},
};

#define NB_COMMANDS (sizeof(commands) / sizeof(command_mapping))

int main(int argc, char *argv[]) {

    //week 9 : initializing vips and shutting it down after the execution
    if (VIPS_INIT(argv[0])) {
        vips_error_exit("unable to start VIPS");
    }
    vips_shutdown();



    if (argc < 2) {
        return ERR_NOT_ENOUGH_ARGUMENTS;
    }

    argc--;
    argv++; // skips command call name
    for (int i = 0; i < NB_COMMANDS; ++i) {
        if (strcmp(argv[0], commands[i].name) == 0) {
            // Call the command function
            argc--;
            argv++;

            //CHANGE ZAC 23.04 ============================================

            int ret = commands[i].command_function(argc, argv);

            if (ret != ERR_NONE) {
                fprintf(stderr, "Error: Invalid usage of command '%s'.\n", commands[i].name);
                help(0, NULL);
                ret = ERR_INVALID_COMMAND;
            }

            return ret;

            //============================================================
        }
    }

    fprintf(stderr, "Unknown command: %s\n", argv[0]);

    help(argc,argv);

    return ERR_INVALID_COMMAND;


}

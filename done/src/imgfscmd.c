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


/*******************************************************************************
 * MAIN
 */

//commands that need to be parsed array !!!
// Command structure
//Typedef for command function pointer
typedef int (*command)(int argc, char* argv[]);

// Struct for command mapping
typedef struct {
    const char* name;
    command command_function;
} command_mapping;


// Array of commands
command_mapping commands[] = {
        {"help", help},
        {"list", do_list_cmd},
        {"create", do_create_cmd},
        {"delete", do_delete_cmd},
};

#define NB_COMMANDS (sizeof(commands) / sizeof(command_mapping))
int main(int argc, char* argv[])
{
    int ret = 0;

    if (argc < 2) {
        ret = ERR_NOT_ENOUGH_ARGUMENTS;
    } else {
        /* **********************************************************************
         * TODO WEEK 07: THIS PART SHALL BE EXTENDED.
         * **********************************************************************
         */

        for (int i = 0; i < NB_COMMANDS; ++i) {
            if (strcmp(argv[1], commands[i].name) == 0) {
                // Call the command function
                ret = commands[i].command_function(argc - 1, argv + 1);
                break;
            }
        }

        if (ret == 0) {
            help(argc, argv);
            ret = ERR_INVALID_COMMAND;
        }

        argc--; argv++; // skips command call name
    }

    if (ret) {
        fprintf(stderr, "ERROR: %s\n", ERR_MSG(ret));
        help(argc, argv);
    }

    return ret;
}

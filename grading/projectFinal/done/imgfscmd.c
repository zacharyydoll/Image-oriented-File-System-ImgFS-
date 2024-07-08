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
#include "util.h"   // for _unused //todo : use the method ?


#include <string.h>
#include <vips/vips.h>

/*******************************************************************************
 * MAIN
 */

//Typedef for command function pointer
typedef int (*command)(int argc, char *argv[]);

//Struct Define a struct that maps command names to their appropriate functions
typedef struct {
    const char *name;
    command command_function;
} command_mapping;


// Array of commands mapped to their functions
command_mapping commands[] = {
    {"help",   help},
    {"list",   do_list_cmd},
    {"create", do_create_cmd},
    {"delete", do_delete_cmd},
    {"insert", do_insert_cmd},
    {"read", do_read_cmd},

};

#define NB_COMMANDS (sizeof(commands) / sizeof(command_mapping)) //The number of commands available in the program

int main(int argc, char *argv[])
{
    int ret ;

    //week 9 : initializing vips and shutting it down after the execution
    if (VIPS_INIT(argv[0])) {
        vips_error_exit("unable to start VIPS");
    }

    if (argc < 2) {
        return ERR_NOT_ENOUGH_ARGUMENTS;
    }

    argc--;
    argv++; // skips command call name

    // Loop through all defined commands
    for (int i = 0; i < (int)NB_COMMANDS; ++i) {
        ret = ERR_INVALID_COMMAND;
        //Search for the command name
        if (strcmp(argv[0], commands[i].name) == 0) {
            // Call the command function
            argc--;
            argv++;

            ret = commands[i].command_function(argc, argv);

            break;

        }
    }

    //If command execution returns an error, show the error message and print help dialog
    if (ret ) {
        fprintf(stderr, "ERROR: %s\n", ERR_MSG(ret));
        help(argc, argv);
    }
    //Shutting down vips
    vips_shutdown();
    return ret;

}

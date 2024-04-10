/**
 * @file imgfscmd_functions.c
 * @brief imgFS command line interpreter for imgFS core commands.
 *
 * @author Mia Primorac
 */

#include "imgfs.h"
#include "imgfscmd_functions.h"
#include "util.h"   // for _unused

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

// default values
static const uint32_t default_max_files = 128;
static const uint16_t default_thumb_res =  64;
static const uint16_t default_small_res = 256;

// max values
static const uint16_t MAX_THUMB_RES = 128;
static const uint16_t MAX_SMALL_RES = 512;

/**********************************************************************
 * Displays some explanations.
 ********************************************************************** */
int help(int useless _unused, char** useless_too _unused)
{
    /* **********************************************************************
     * TODO WEEK 08: WRITE YOUR CODE HERE.
     * **********************************************************************
     */

    TO_BE_IMPLEMENTED();
    return NOT_IMPLEMENTED;
}

/**********************************************************************
 * Opens imgFS file and calls do_list().
 ********************************************************************** */
int do_list_cmd(int argc, char** argv) {
    M_REQUIRE_NON_NULL(argv);
    //argc should contain program name, list command, and exactly one filename
    if (argc != 3)
        return ERR_INVALID_COMMAND;

    imgfs_file imgfsFile; // initialize structure
    memset(&imgfsFile, 0, sizeof(imgfsFile)); //make sure all bytes are set to 0

    //Open
    int open_ret = do_open(argv[2], "rb", &imgfsFile); // rb because no modifications
    if (open_ret != ERR_NONE) return open_ret; //return ERR_IO or ERR_OUT_OF_MEMORY if file can't be opened

    //Display
    int list_ret = do_list(&imgfs, STDOUT, NULL);
    if (list_ret != ERR_NONE) {
        do_close(&imgfs);  // close the file before returning
        return list_ret;
    }

    //Close
    do_close(&imgfsFile);

    return ERR_NONE;
}

/**********************************************************************
 * Prepares and calls do_create command.
********************************************************************** */
int do_create_cmd(int argc, char** argv)
{

    puts("Create");
    /* **********************************************************************
     * TODO WEEK 08: WRITE YOUR CODE HERE (and change the return if needed).
     * **********************************************************************
     */

    TO_BE_IMPLEMENTED();
    return NOT_IMPLEMENTED;
}

/**********************************************************************
 * Deletes an image from the imgFS.
 */
int do_delete_cmd(int argc, char** argv)
{
    /* **********************************************************************
     * TODO WEEK 08: WRITE YOUR CODE HERE (and change the return if needed).
     * **********************************************************************
     */

    TO_BE_IMPLEMENTED();
    return NOT_IMPLEMENTED;
}

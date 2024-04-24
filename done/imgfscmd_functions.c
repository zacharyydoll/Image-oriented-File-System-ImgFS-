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
static const uint16_t default_thumb_res = 64;
static const uint16_t default_small_res = 256;

// max values
static const uint16_t MAX_THUMB_RES = 128;
static const uint16_t MAX_SMALL_RES = 512;

/**********************************************************************
 * Displays some explanations.
 ********************************************************************** */
int help(int useless _unused, char **useless_too _unused) {
    printf("imgfscmd [COMMAND] [ARGUMENTS]\n"
           "  help: displays this help.\n"
           "  list <imgFS_filename>: list imgFS content.\n"
           "  create <imgFS_filename> [options]: create a new imgFS.\n"
           "      options are:\n"
           "          -max_files <MAX_FILES>: maximum number of files.\n"
           "                                  default value is %u\n"
           "                                  maximum value is 4294967295\n"
           "          -thumb_res <X_RES> <Y_RES>: resolution for thumbnail images.\n"
           "                                  default value is %ux%u\n"
           "                                  maximum value is %ux%u\n"
           "          -small_res <X_RES> <Y_RES>: resolution for small images.\n"
           "                                  default value is %ux%u\n"
           "                                  maximum value is %ux%u\n"
           "  delete <imgFS_filename> <imgID>: delete image imgID from imgFS.\n",
           default_max_files,
           default_thumb_res, default_thumb_res,
           MAX_THUMB_RES, MAX_THUMB_RES,
           default_small_res, default_small_res,
           MAX_SMALL_RES, MAX_SMALL_RES
    );

    return ERR_NONE;
}

/**********************************************************************
 * Opens imgFS file and calls do_list().
 ********************************************************************** */
int do_list_cmd(int argc, char **argv) {
    M_REQUIRE_NON_NULL(argv);
    //argc should contain program name, list command, and exactly one filename

    //CHANGE_SARA should != 1 instead of !=3
    if (argc != 1)
        return ERR_INVALID_COMMAND;

    // initialize structure, set all bytes to 0
    //CHANGE_SARA : changed struct imgfs_file imgfsFile = NULL to this line because can't cast NULL to struct imgfs_file
    struct imgfs_file imgfsFile ;
    memset(&imgfsFile, 0, sizeof(imgfsFile));

    //Open
    //CHANGE_SARA should be argv[0] instead of argv[3]
    int open_ret = do_open(argv[0], "rb", &imgfsFile);
    if (open_ret != ERR_NONE) return open_ret;

    //Display
    int list_ret = do_list(&imgfsFile, STDOUT, NULL);
    if (list_ret != ERR_NONE) {
        do_close(&imgfsFile); // close the file before returning
        return list_ret;
    }

    //Close
    do_close(&imgfsFile);

    return ERR_NONE;
}

/**********************************************************************
 * Prepares and calls do_create command.
********************************************************************** */
int do_create_cmd(int argc, char **argv) {

    M_REQUIRE_NON_NULL(argv);
    //CHANGE ZAC 23.04 : changed from 4 to 1 (only mandatory argument is the file name)
    if (argc < 1) {
        // file name argument is mandatory
        return ERR_NOT_ENOUGH_ARGUMENTS;
    }
    if (argv[0] == NULL) {
        return ERR_INVALID_FILENAME;
    }

    //CHANGE ZAC 23.04 : changed from 2 to 0 (the mandatory argument of the filename is the first to be parsed)
    const char *imgfs_filename = argv[0];
    int max_files = default_max_files;
    int thumb_x_res = default_thumb_res;
    int thumb_y_res = default_thumb_res;
    int small_x_res = default_small_res;
    int small_y_res = default_small_res;

    // CHANGES ZAC =========================================================================================================

    // first 3 arguments are mandatory
    for (int i = 1; i < argc; ++i) {
        char *curr = argv[i]; // Current (optional) argument
        
        if (strcmp(curr, "-max_files") == 0 && i + 1 < argc) { //checks that we have at least one parameter
            max_files = (uint32_t) atouint32(argv[++i]);  // Move to next argument and convert to integer
            if (max_files == 0) {
                return ERR_INVALID_ARGUMENT;
            }
        } else if (strcmp(curr, "-thumb_res") == 0 && i + 2 < argc) { //checks that we have at least two parameters
            thumb_x_res = (uint16_t) atouint16(argv[++i]);
            thumb_y_res = (uint16_t) atouint16(argv[++i]);
            if (thumb_x_res == 0 || thumb_y_res == 0 || thumb_x_res > MAX_THUMB_RES || thumb_y_res > MAX_THUMB_RES) {
                return ERR_RESOLUTIONS;
            }
        } else if (strcmp(curr, "-small_res") == 0 && i + 2 < argc) {  //checks that we have at least two parameters
            small_x_res = (uint16_t) atouint16(argv[++i]);
            small_y_res = (uint16_t) atouint16(argv[++i]);
            if (small_x_res == 0 || small_y_res == 0 || small_x_res > MAX_SMALL_RES || small_y_res > MAX_SMALL_RES) {
                return ERR_RESOLUTIONS;
            }
        } else {
            return ERR_INVALID_ARGUMENT;  // Argument is not recognized or missing necessary parameters
        }
    }

    // =====================================================================================================================

    // initialize imgfs_file structure with the given parameters and call do_create to initialize rest
    struct imgfs_file imgfsFile = {
        .header = {
            .max_files = max_files,
            .resized_res = {thumb_x_res, thumb_y_res, small_x_res, small_y_res}
        }
    };
    do_create(imgfs_filename, &imgfsFile);

    return ERR_NONE;
}

/**********************************************************************
 * Deletes an image from the imgFS.
 */
int do_delete_cmd(int argc, char **argv) {

    M_REQUIRE_NON_NULL(argv);

    if (argc != 2) {
        //delete requires <imgFS_filename> and <imgID> as arguments
        return ERR_NOT_ENOUGH_ARGUMENTS;
    }

    const char *imgfs_filename = argv[0];
    const char *imgID = argv[1];


    if (strlen(imgID)>MAX_IMG_ID || imgID == NULL) {
        //CHANGE ZAC 23.04 : modified return error to fit handout requirement
        return ERR_INVALID_IMGID;
    }

    // initializing structure and setting all bytes to 0
    struct imgfs_file imgfsFile;
    memset(&imgfsFile, 0, sizeof(imgfsFile));

    //Open
    int open_ret = do_open(imgfs_filename, "rb+", &imgfsFile);
    //in case of error
    if (open_ret != ERR_NONE) return open_ret;


    //Delete
    int delete_ret = do_delete(imgID, &imgfsFile);
    if (delete_ret != ERR_NONE) {
        do_close(&imgfsFile);
        fprintf(stderr, "error code in do_delete_cmd: %d\n", delete_ret);
        return delete_ret;
    }


    //Close
    do_close(&imgfsFile);


    return ERR_NONE;
}

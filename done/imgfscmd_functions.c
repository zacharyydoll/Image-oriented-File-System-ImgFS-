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
int help(int useless _unused, char **useless_too _unused)
{
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
int do_list_cmd(int argc, char **argv)
{
    //Argument validity check
    M_REQUIRE_NON_NULL(argv);

    //Command validity check
    if (argc != 1)
        return ERR_INVALID_COMMAND;

    // Initialize the imgFS file structure and set all bytes to 0
    struct imgfs_file imgfsFile ;
    memset(&imgfsFile, 0, sizeof(imgfsFile));

    //Opening imgFS file
    int open_ret = do_open(argv[0], "rb", &imgfsFile);
    if (open_ret != ERR_NONE) return open_ret;

    //Displaying the content of the imgFS file
    int list_ret = do_list(&imgfsFile, STDOUT, NULL);

    if (list_ret != ERR_NONE) {
        do_close(&imgfsFile); // closing the file before returning
        return list_ret;
    }

    //Closing the imgFS file
    do_close(&imgfsFile);

    //Everything gone well
    return ERR_NONE;
}

/**
 * @brief Creates a new imgFS with the specified parameters.
 *
 * This function creates a new imgFS file with the given file name and initializes its header and metadata.
 * The function processes the optional arguments for max_files, thumb_res, and small_res and sets the corresponding
 * values in the header. The function then calls the do_create function to initialize the rest of the imgFS file.
 *
 * @param argc The number of command-line arguments.
 * @param argv An array of command-line argument strings.
 * @return An error code indicating the outcome of the operation.
 *         - ERR_NONE: The imgFS file was created successfully.
 *         - ERR_INVALID_ARGUMENT: An argument is not recognized or missing necessary parameters.
 *         - ERR_NOT_ENOUGH_ARGUMENTS: Not enough arguments provided.
 *         - ERR_INVALID_FILENAME: The file name argument is invalid or missing.
 * @note The argument with index 0 in argv is expected to be the file name.
 */
int do_create_cmd(int argc, char **argv)
{
    //Argument validity check
    M_REQUIRE_NON_NULL(argv);

    //Argument number validity check
    if (argc < 1) {
        // file name argument is mandatory
        return ERR_NOT_ENOUGH_ARGUMENTS;
    }

    //Filename validity check
    if (argv[0] == NULL) {
        return ERR_INVALID_FILENAME;
    }

    // Processing the optional arguments for max_files, thumb_res and small_res
    const char *imgfs_filename = argv[0];
    uint32_t max_files = default_max_files;
    uint16_t thumb_x_res = default_thumb_res;
    uint16_t thumb_y_res = default_thumb_res;
    uint16_t small_x_res = default_small_res;
    uint16_t small_y_res = default_small_res;

    // =========================================================================================================

    for (int i = 1; i < argc; ++i) {
        char *curr = argv[i]; // Current (optional) argument

        if (strcmp(curr, "-max_files") == 0 ) {//checks that we have at least one parameter
            if (i + 1 >= argc) {
                return ERR_NOT_ENOUGH_ARGUMENTS;
            }
            max_files = atouint32(argv[++i]);  // Move to next argument and convert to integer
            if (max_files == 0) {
                return ERR_INVALID_ARGUMENT;
            }
        } else if (strcmp(curr, "-thumb_res") == 0 ) {//checks that we have at least two parameters
            if (i + 2 >= argc) {
                return ERR_NOT_ENOUGH_ARGUMENTS;
            }
            thumb_x_res =  atouint16(argv[++i]);
            thumb_y_res =  atouint16(argv[++i]);

            if (thumb_x_res == 0 || thumb_y_res == 0 || thumb_x_res > MAX_THUMB_RES || thumb_y_res > MAX_THUMB_RES) {
                return ERR_RESOLUTIONS;
            }
        } else if (strcmp(curr, "-small_res") == 0 ) {//checks that we have at least two parameters
            if (i + 2 >= argc) {
                return ERR_NOT_ENOUGH_ARGUMENTS;
            }
            small_x_res =  atouint16(argv[++i]);
            small_y_res =  atouint16(argv[++i]);
            if (small_x_res == 0 || small_y_res == 0 || small_x_res > MAX_SMALL_RES || small_y_res > MAX_SMALL_RES) {
                return ERR_RESOLUTIONS;
            }
        } else {
            return ERR_INVALID_ARGUMENT;  // Argument is not recognized or missing necessary parameters
        }
    }


    // =====================================================================================================================

    // Initializing imgfs_file structure with the given parameters to call do_create
    struct imgfs_file imgfsFile = {
        .header = {
            .max_files = max_files,
            .resized_res = {thumb_x_res, thumb_y_res, small_x_res, small_y_res}
        }
    };

    do_create(imgfs_filename, &imgfsFile);

    // After the imgFS file is created, for cleanup, the file is closed and the metadata is freed
    fclose(imgfsFile.file);
    free(imgfsFile.metadata);

    //Everything went well
    return ERR_NONE;
}

/**********************************************************************
 * Deletes an image from the imgFS.
 */
int do_delete_cmd(int argc, char **argv)
{

    //Argument validity check
    M_REQUIRE_NON_NULL(argv);

    //Argument number check
    if (argc != 2) {
        //delete requires <imgFS_filename> and <imgID> as arguments
        return ERR_NOT_ENOUGH_ARGUMENTS;
    }

    // Getting the filename of the imgFS and the image ID from the arguments
    const char *imgfs_filename = argv[0];
    const char *imgID = argv[1];

    //ImgId validity check
    if (strlen(imgID)>MAX_IMG_ID || imgID == NULL) {
        return ERR_INVALID_IMGID;
    }

    // initializing structure and setting all bytes to 0
    struct imgfs_file imgfsFile;
    memset(&imgfsFile, 0, sizeof(imgfsFile));

    //Opening the file in rb+ mode
    int open_ret = do_open(imgfs_filename, "rb+", &imgfsFile);
    //In case of error
    if (open_ret != ERR_NONE) return open_ret;


    // Using the do_delete function to delete the specified image from the imgFS file
    int delete_ret = do_delete(imgID, &imgfsFile);
    if (delete_ret != ERR_NONE) {
        do_close(&imgfsFile);
        return delete_ret;
    }


    // Close the imgFS file after successful deletion
    do_close(&imgfsFile);

    //Everything went well
    return ERR_NONE;
}

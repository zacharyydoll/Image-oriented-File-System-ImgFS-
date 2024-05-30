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


// default values
static const uint32_t default_max_files = 128;
static const uint16_t default_thumb_res = 64;
static const uint16_t default_small_res = 256;

// max values
static const uint16_t MAX_THUMB_RES = 128;
static const uint16_t MAX_SMALL_RES = 512;

/**
 * @description Creates new file's name based on img id and resolution.
 * @param img_id A pointer to the unique id of the image.
 * @param resolution The resolution of the image for which the name is to be created.
 * @param new_name Address of a character pointer where the new name of the image would be stored
 */
static void create_name(const char* img_id, int resolution, char** new_name);
/**
 * @description Write a disk image's buffer to a specified file.
 * @param filename A pointer the name of the file to which the image buffer is to be written.
 * @param image_buffer A pointer the buffer data of the disk image.
 * @param image_size The size of the image_buffer in bytes.
 * @returns Integer value indicating the status of the write operation : 0 if everything works fine
 */
static int write_disk_image(const char *filename, const char *image_buffer, uint32_t image_size);

/**
 * @description Reads disk image data from a specified file into a buffer.
 * @param path A pointer to the path of the file from which the image data is to be read.
 * @param image_buffer Address of the buffer where the read image data would be stored.
 * @param image_size Pointer where the size of the image data read would be stored.
 * @returns Integer value indicating the status of the read operation : 0 if everything works fine
 */
static int read_disk_image(const char *path, char **image_buffer, uint32_t *image_size);

/**********************************************************************
 * Displays some explanations -> Updated in week 10
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
           "  read   <imgFS_filename> <imgID> [original|orig|thumbnail|thumb|small]:\n"
           "      read an image from the imgFS and save it to a file.\n"
           "      default resolution is \"original\".\n"
           "  insert <imgFS_filename> <imgID> <filename>: insert a new image in the imgFS.\n"
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

    //==================================================================================================================

    for (int i = 1; i < argc; ++i) {
        char *curr = argv[i]; // Current (optional) argument

        if (strcmp(curr, "-max_files") == 0 ) { //checks that we have at least one parameter
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


    //==================================================================================================================

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


/**********************************************************************
 * Reads an image from the imgFS and writes it to a separate image file.
 */
int do_read_cmd(int argc, char **argv)
{
    M_REQUIRE_NON_NULL(argv);
    if (argc != 2 && argc != 3) return ERR_NOT_ENOUGH_ARGUMENTS;

    const char * const img_id = argv[1];

    const int resolution = (argc == 3) ? resolution_atoi(argv[2]) : ORIG_RES;
    if (resolution == -1) return ERR_RESOLUTIONS;

    struct imgfs_file myfile;
    zero_init_var(myfile);
    int error = do_open(argv[0], "rb+", &myfile);

    if (error != ERR_NONE) return error;

    char *image_buffer = NULL;
    uint32_t image_size = 0;
    error = do_read(img_id, resolution, &image_buffer, &image_size, &myfile);

    do_close(&myfile);
    if (error != ERR_NONE) {
        return error;
    }

    // Extracting to a separate image file.
    char* tmp_name = NULL;
    create_name(img_id, resolution, &tmp_name);
    if (tmp_name == NULL) return ERR_OUT_OF_MEMORY;
    error = write_disk_image(tmp_name, image_buffer, image_size);
    free(tmp_name);
    free(image_buffer);

    return error;
}


/**********************************************************************
 * Inserts an image into the imgFS.
 */
int do_insert_cmd(int argc, char **argv)
{
    M_REQUIRE_NON_NULL(argv);
    if (argc != 3) return ERR_NOT_ENOUGH_ARGUMENTS;

    struct imgfs_file imgfsFile;
    zero_init_var(imgfsFile);
    int error = do_open(argv[0], "rb+", &imgfsFile);
    if (error != ERR_NONE) return error;

    char *image_buffer = NULL;
    uint32_t image_size;

    // Reads image from the disk.
    error = read_disk_image (argv[2], &image_buffer, &image_size);
    if (error != ERR_NONE) {
        do_close(&imgfsFile);
        return error;
    }

    error = do_insert(image_buffer, image_size, argv[1], &imgfsFile);
    free(image_buffer);
    do_close(&imgfsFile);
    return error;
}

/**********************************************************************
 * Create a new name for the image file.
 */
static void create_name(const char* img_id, int resolution, char** new_name) {

    const char* resolution_str = NULL;

    switch (resolution) {
        case ORIG_RES:
            resolution_str = "_orig";
            break;
        case THUMB_RES:
            resolution_str = "_thumb";
            break;
        case SMALL_RES:
            resolution_str = "_small";
            break;
        default:
            resolution_str = "_unknownResolution";
    }

    //Allocating memory for : image_id + resolution_suffix + '.jpg' + '\0'
    *new_name = calloc(strlen(img_id) + strlen(resolution_str) + strlen(".jpg") + 1, sizeof(char));
    if (*new_name == NULL) {
        return ; // out of memory
    }

    //Using sprintf to update filename
    sprintf(*new_name, "%s%s.jpg", img_id, resolution_str);
}

/**********************************************************************
 * Write the content of a buffer of size provided, to a file of name provided.

 */
static int write_disk_image(const char *filename, const char *image_buffer, uint32_t image_size) {

    //Arguments validity check
    M_REQUIRE_NON_NULL(filename);
    M_REQUIRE_NON_NULL(image_buffer);

    FILE *file = fopen(filename, "wb");
    if (file == NULL) return ERR_IO; // Checking if file opening failed

    //Writing images to disk
    size_t written_bytes = fwrite(image_buffer, sizeof(char), image_size, file);

    fclose(file);

    if (written_bytes != image_size) {
        fclose(file);
        return ERR_IO;
    }

    return ERR_NONE;
}



/**********************************************************************
 * Read content of file to buffer.

 */
static int read_disk_image(const char *path, char **image_buffer, uint32_t *image_size) {

    //Argument validity check
    M_REQUIRE_NON_NULL(path);
    M_REQUIRE_NON_NULL(image_buffer);
    M_REQUIRE_NON_NULL(image_size);

    FILE *file = fopen(path, "rb");
    if (!file) {
        return ERR_IO;
    }

    //Seeking end of file to get its size
    fseek(file, 0, SEEK_END);
    *image_size = ftell(file);
    //Going back to start of file
    fseek(file, 0, SEEK_SET);

    //Allocating memory for image
    *image_buffer = (char *)malloc(*image_size);
    if (!(*image_buffer)) {
        fclose(file);
        return ERR_OUT_OF_MEMORY;
    }

    size_t read = fread(*image_buffer, 1, *image_size, file); //read file into buffer
    fclose(file);

    if (read != *image_size) {
        free(*image_buffer);
        return ERR_IO;
    }

    return ERR_NONE;
}




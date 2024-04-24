#include "imgfs.h"
#include <string.h> // for strncmp
#include "util.h"

int do_delete(const char *img_id, struct imgfs_file *imgfs_file) {
    M_REQUIRE_NON_NULL(img_id);
    M_REQUIRE_NON_NULL(imgfs_file);
    if (imgfs_file->file == NULL) {
        return ERR_IO;
    }

    //================== CHANGES ZAC 22.04 ==============================================================================
    int found_flag = 0;
    uint32_t idx = 0;

    //search for image with the given img_id
    for (idx = 0; idx < imgfs_file->header.max_files; ++idx) {
        // Added condition to check if the image is valid
        if (strncmp(imgfs_file->metadata[idx].img_id, img_id, MAX_IMG_ID) == 0 &&
            imgfs_file->metadata[idx].is_valid != EMPTY) {
            imgfs_file->metadata[idx].is_valid = EMPTY; //invalidate the image if it was found
            found_flag = 1;
            break;
        }
    }

    if (!found_flag) {
        //if no image corresponding image was found, directly return error
        printf("Image not found\n");
        return ERR_IMAGE_NOT_FOUND;
    }

    //Write updated metadata to disk
    fseek(imgfs_file->file, sizeof(struct imgfs_header) + sizeof(struct img_metadata) * idx, SEEK_SET);
    if (fwrite(&imgfs_file->metadata[idx], sizeof(struct img_metadata), 1, imgfs_file->file) != 1) {
        return ERR_IO;
    }
    fflush(imgfs_file->file);

    //Update header information
    imgfs_file->header.nb_files--;
    imgfs_file->header.version++;

    //Write updated header to disk
    fseek(imgfs_file->file, 0, SEEK_SET);
    if (fwrite(&imgfs_file->header, sizeof(struct imgfs_header), 1, imgfs_file->file) != 1) {
        return ERR_IO;
    }
    fflush(imgfs_file->file);


    return ERR_NONE;

    //===================================================================================================================

}



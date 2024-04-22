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
            printf("Image Found: %s\n", img_id);
            imgfs_file->metadata[idx].is_valid = EMPTY; //invalidate the image if it was found
            found_flag = 1;
            break;
        }
    }

    if (!found_flag) {
        //if no image corresponding image was found, directly return error
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
    // Old version commented (for comparison if necessary)
    /*for (uint32_t i = 0; i < imgfs_file->header.max_files; ++i) {
        // find the image reference with the same name in the "metadata" and invalidate it (see handout)
        if (strncmp(imgfs_file->metadata[i].img_id, img_id, MAX_IMG_ID) == 0 && imgfs_file->metadata[i].is_valid != EMPTY) {
            //imgfs_file->metadata[i].is_valid = EMPTY;


            //------------------------CHANGES -----------------------------------------------------------------------------------
            //writing changes to disk using file functions (whole metadata for compatibility)
            fseek(imgfs_file->file, sizeof(struct imgfs_header), SEEK_SET);
            fwrite(&(imgfs_file->metadata), sizeof(struct imgfs_file), imgfs_file->header.nb_files, imgfs_file->file);

            //updating header information
            imgfs_file->header.nb_files--;
            imgfs_file->header.version++;

            //writing the header into the disk after the update
            fseek(imgfs_file->file, 0, SEEK_SET);
            fwrite(&(imgfs_file->header), sizeof(struct imgfs_header), 1, imgfs_file->file);

            //----------------------------------------------------------------------------------------------------------------------
            return ERR_NONE;
        }*/
}



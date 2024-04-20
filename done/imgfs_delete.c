#include "imgfs.h"
#include <string.h> // for strncmp

int do_delete(const char *img_id, struct imgfs_file *imgfs_file) {
    M_REQUIRE_NON_NULL(img_id);
    M_REQUIRE_NON_NULL(imgfs_file);
    if (imgfs_file->file == NULL) {
        return ERR_IO;
    }

    //search for image with the given img_id
    for (uint32_t i = 0; i < imgfs_file->header.max_files; ++i) {
        // find the image reference with the same name in the "metadata" and invalidate it (see handout)
        if (strncmp(imgfs_file->metadata[i].img_id, img_id, MAX_IMG_ID) == 0) {
            imgfs_file->metadata[i].is_valid = EMPTY;

            //------------------------THE CHANGES-----------------------------------------------------------------------------------
            //writing changes to disk using file functions (whole metadata for compability)
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
        }
    }


    /*
     * TODO : - write changes to disk using fseek() and then fwrite()
     *        - handle case where image database does not exist
     *        - update header if operation is successful :
     *             - increase imgfs_version by 1
     *             - adjust nb_files
     *        - write header to disk
     */



    //case when database is not found
    return ERR_IMAGE_NOT_FOUND;
}

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

    return ERR_NOT_FOUND;
}

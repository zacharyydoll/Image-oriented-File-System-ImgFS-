#include "image_dedup.h"
#include <string.h> // for memcmp

int do_name_and_content_dedup(struct imgfs_file *imgfs_file, uint32_t index) {

    M_REQUIRE_NON_NULL(imgfs_file);
    if (index >= imgfs_file->metadata.nb_files) {
        return ERR_OUT_OF_BOUND;
    }

    img_metadata *targetImg = &imgfs_file->metadata[index];
    if (!targetImg->is_valid) return ERR_OUT_OF_BOUND; //if image is invalid, return out of bounds (see tests

    for (uint32_t i = 0; i < imgfs_file->metadata.nb_files; ++i) {

        if (i != index) {
            struct img_metadata *currImg = imgfs_file->metadata[i];

            //ensure that the image database does not contain two images with the same internal identifier (handout)
            if (strncmp(currImg->img_id, imgfs_file->metadata->img_id, MAX_IMG_ID) == 0) {
                return ERR_DUPLICATE_ID;
            }
            if (!currImg->is_valid) return ERR_OUT_OF_BOUND; //if image is invalid, return out of bounds (see tests)

            //used memcmp instead of strcmp (SHA can refer to string or content -> compare raw data with memcmp)
            if (memcmp(imgfs_file->metadata[i].SHA,
                       imgfs_file->metadata[index].SHA,
                       SHA256_DIGEST_LENGTH) == 0) {

                //new image (at position index) now references the same content as the image at position i (handout)
                targetImg->orig_res[0] = currImg->orig_res[0];
                targetImg->orig_res[1] = currImg->orig_res[1];
                for (int resolution = 0; resolution < NB_RES; resolution++) {
                    targetImg->size[resolution] = currImg->size[resolution];
                    targetImg->offset[resolution] = currImg->offset[resolution];
                }
                //return so that only first valid duplicate updates the references, avoids useless iterations
                return ERR_NONE;
            }
        }
    }
    //case where image has no duplicate content, and no duplicate id
    targetImg->orig_res[0] = 0;
    targetImg->orig_res[1] = 0;

    return ERR_NONE;
}
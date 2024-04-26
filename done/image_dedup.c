#include "image_dedup.h"
#include <string.h> // for memcmp

int do_name_and_content_dedup(struct imgfs_file *imgfs_file, uint32_t index) {
    M_REQUIRE_NON_NULL(imgfs_file);
    //CHANGE_SARA 26/04 : Updated the condition to > instead of >=
    if (index > imgfs_file->header.nb_files) {
        return ERR_IMAGE_NOT_FOUND;
    }

    struct img_metadata *targetImg = &imgfs_file->metadata[index];
    //CHANGE_SARA 26/04 : Updated the condition to == EMPTY...which is better ?
    if (targetImg->is_valid == EMPTY) {
        return ERR_IMAGE_NOT_FOUND; //if image is invalid, return out of bounds (see tests)
    }

    for (uint32_t i = 0; i < imgfs_file->header.nb_files; ++i) {

        if (i != index) {
            struct img_metadata *currImg = &imgfs_file->metadata[i];


            //ensure that the image database does not contain two images with the same internal identifier (handout)
            //CHANGE_SARA 26/04 : Updated the condition to metadata[index]
            if (strncmp(currImg->img_id, imgfs_file->metadata[index].img_id, MAX_IMG_ID) == 0) {
                return ERR_DUPLICATE_ID;
            }

            //used memcmp instead of strcmp (SHA can refer to string or content -> compare raw data with memcmp)
            if (memcmp(imgfs_file->metadata[i].SHA,
                       imgfs_file->metadata[index].SHA,
                       SHA256_DIGEST_LENGTH) == 0) {

                //new image (at position index) now references the same content as the image at position i (handout)
                // Modify the metadata at the index position
                //CHANGE_SARA 26/04 : Updated method of modification of offset (handout + ED ##996)
                for (int resolution = THUMB_RES; resolution < NB_RES; resolution++) {
                    targetImg->size[resolution] = currImg->size[resolution];
                    targetImg->offset[resolution] = currImg->offset[resolution];
                }

                //return so that only first valid duplicate updates the references, avoids useless iterations
                return ERR_NONE;
            }
        }
    }
    //case where image has no duplicate content, and no duplicate id
    //CHANGE_SARA 26/04 : Updated the case with no duplicate id but not sure about it
    targetImg->offset[ORIG_RES] = 0;

    return ERR_NONE;
}
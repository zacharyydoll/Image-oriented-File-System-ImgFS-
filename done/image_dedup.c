#include "image_dedup.h"
#include <string.h> // for memcmp

int do_name_and_content_dedup(struct imgfs_file *imgfs_file, uint32_t index)
{
    //Argument validity check
    M_REQUIRE_NON_NULL(imgfs_file);

    //Index validity check
    if (index > imgfs_file->header.nb_files) {
        return ERR_IMAGE_NOT_FOUND;
    }

    // Getting metadata of the target image
    struct img_metadata *targetImg = &imgfs_file->metadata[index];

    /*
    if (targetImg->is_valid == EMPTY) {
        printf("\nhere in dedup 2\n");
        return ERR_IMAGE_NOT_FOUND; //if image is invalid, return out of bounds (see tests)
    }
     */

    for (uint32_t i = 0; i < imgfs_file->header.nb_files; ++i) {

        struct img_metadata *currImg = &imgfs_file->metadata[i];

        // Image must be ignored if empty
        if (i != index && currImg->is_valid != EMPTY) {
            // Check for duplicate internal identifiers
            if (strcmp(currImg->img_id, imgfs_file->metadata[index].img_id) == 0) {
                return ERR_DUPLICATE_ID;
            }

            //Using memcmp instead of strcmp (SHA can refer to string or content -> compare raw data with memcmp)
            if (memcmp(imgfs_file->metadata[i].SHA,
                       imgfs_file->metadata[index].SHA,
                       SHA256_DIGEST_LENGTH) == 0) {


                // Update metadata of the target image to reference the same content as the duplicate image
                for (int resolution = THUMB_RES; resolution < NB_RES; resolution++) {
                    targetImg->size[resolution] = currImg->size[resolution];
                    targetImg->offset[resolution] = currImg->offset[resolution];
                }

                //return so that only first valid duplicate updates the references, avoids useless iterations
                return ERR_NONE;
            }
        }
    }

    //Case where image has no duplicate content, and no duplicate id
    targetImg->offset[ORIG_RES] = 0;
    return ERR_NONE;
}

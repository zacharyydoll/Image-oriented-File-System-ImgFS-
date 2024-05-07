#include "imgfs.h"
#include <string.h>
#include "image_content.h"
#include <stdlib.h>

int do_read(const char *img_id, int resolution, char **image_buffer,
            uint32_t *image_size, struct imgfs_file *imgfs_file) {

    printf("\nwe are in do read function\n");

    M_REQUIRE_NON_NULL(img_id);
    M_REQUIRE_NON_NULL(image_buffer);
    M_REQUIRE_NON_NULL(image_size);
    M_REQUIRE_NON_NULL(imgfs_file);

    //find entry in metadata corresponding to supplied imgID
    int imgID_idx ;
    //CHANGE_SARA changed the while to for to unsure break to pass test
    for (imgID_idx = 0; imgID_idx < imgfs_file->header.max_files; ++imgID_idx) {
        if (strncmp(imgfs_file->metadata[imgID_idx].img_id, img_id, MAX_IMG_ID) == 0) {
            break;
        }
    }

    printf("after for \n");

    //case where no corresponding imgID was found in metadata table

    if(imgID_idx == imgfs_file->header.max_files) {
        printf("Image not found\n");
        return ERR_IMAGE_NOT_FOUND;
    }


    printf("before if \n");

    if(resolution != ORIG_RES) {

        //if image does not already exist in requested resolution, call lazily_resize (if not original resolution)
        //CHANGE_SARA null instead of 0 (see handout)
        if (!(&imgfs_file->metadata[imgID_idx].offset[resolution] == NULL ||
              &imgfs_file->metadata[imgID_idx].size[resolution] == NULL)) {

            printf("inside if\n");
            int ret_resize = lazily_resize(resolution, imgfs_file, imgID_idx);
            if (ret_resize != ERR_NONE) {
                return ret_resize;
            }
        }
    }


    //read the content of the image into buffer, now that we have index and size
    *image_size = imgfs_file->metadata[imgID_idx].size[resolution];
    *image_buffer = calloc(1,*image_size);
    if(*image_buffer == NULL) {
        return ERR_OUT_OF_MEMORY;
    }

    if(fread(*image_buffer, *image_size, 1, imgfs_file->file) != 1) {
        free(*image_buffer);
        return ERR_IO;
    }

    return ERR_NONE;
}
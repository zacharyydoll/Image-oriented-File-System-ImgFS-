#include "imgfs.h"
#include <string.h> // for strncpy
#include "image_dedup.h" // for do_name_and_content_dedup()
#include "image_content.h" // for get_resolution()


int do_insert(const char *image_buffer, size_t image_size,
              const char *img_id, struct imgfs_file *imgfs_file) {

    M_REQUIRE_NON_NULL(img_id);
    M_REQUIRE_NON_NULL(image_buffer);
    M_REQUIRE_NON_NULL(imgfs_file);

    if(imgfs_file->header.nb_files >= imgfs_file->header.max_files) {
        return ERR_IMGFS_FULL;
    }

    //Find empty entry index in metadata table
    uint32_t free_idx = -1;
    while(++free_idx < imgfs_file->header.max_files && !imgfs_file->metadata[free_idx].is_valid);
    if(free_idx == -1) return ERR_IMGFS_FULL;

    SHA256((const unsigned char *)image_buffer, image_size, imgfs_file->metadata[free_idx].SHA);
    strncpy(imgfs_file->metadata[free_idx].img_id, img_id, MAX_IMG_ID);
    imgfs_file->metadata[free_idx].size[ORIG_RES] = (uint32_t) image_size; // Not sure cast is completely correct.

    //Put correct height and width in the image's metadata (see image_content.h #get_resolution())
    int ret_resolution = get_resolution(&imgfs_file->metadata[free_idx].orig_res[0],
                                        &imgfs_file->metadata[free_idx].orig_res[1],
                                        image_buffer, image_size);
    if(ret_resolution != ERR_NONE) {
        return ret_resolution;
    }

    // Remove duplicates
    int ret_dedup = do_name_and_content_dedup(imgfs_file, free_idx);

    //Check whether dedup has found a duplicate (see bottom of image_dedup.c #do_name_and_content_dedup())
    if(imgfs_file->metadata[free_idx].offset[ORIG_RES] != 0) {
        return ERR_NONE;
    }

    //If no duplicates were found, write the image to the end of the file
    printf("Attempting to insert: %s with index %u\n", img_id, free_idx);
    if (ret_dedup != ERR_NONE) {
        printf("Deduplication error: %d\n", ret_dedup);
        return ret_dedup;
    }


    fseek(imgfs_file->file, 0, SEEK_END);
    if(fwrite(image_buffer, image_size, 1, imgfs_file->file) != -1) {
        return ERR_IO;
    }

    //finish initializing the metadata
    imgfs_file->metadata[free_idx].offset[ORIG_RES] = ftell(imgfs_file->file);
    imgfs_file->metadata[free_idx].is_valid = NON_EMPTY;

    //Update all the necessary image database header fields.
    imgfs_file->header.nb_files++;
    imgfs_file->header.version++;

    //write header to disk, and then corresponding metadata (but not all of it)
    fseek(imgfs_file->file, 0, SEEK_SET);
    if(fwrite(&imgfs_file->header, sizeof(struct imgfs_header), 1, imgfs_file->file) != 1) {
        return ERR_IO;
    }
    if(fwrite(&imgfs_file->metadata[free_idx],sizeof(struct img_metadata),1, imgfs_file->file) != 1) {
        return ERR_IO;
    }

    return ERR_NONE;
}

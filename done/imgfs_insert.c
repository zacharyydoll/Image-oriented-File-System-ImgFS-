#include "imgfs.h"
#include <string.h> // for strncpy
#include "image_dedup.h" // for do_name_and_content_dedup()
#include "image_content.h" // for get_resolution()


int do_insert(const char *image_buffer, size_t image_size,
              const char *img_id, struct imgfs_file *imgfs_file) {
    //change null
    if (image_size == 0){return ERR_INVALID_ARGUMENT;}
    M_REQUIRE_NON_NULL(img_id);
    M_REQUIRE_NON_NULL(image_buffer);
    M_REQUIRE_NON_NULL(imgfs_file);

    if(imgfs_file->header.nb_files >= imgfs_file->header.max_files) {
        return ERR_IMGFS_FULL;
    }

    //Find empty entry index in metadata table
    int free_idx = -1;
    //CHANGE_SARA : changed while to for to make sure the loops breaks i don't understand why it doesn't work with while
    for (int i = 0; i < imgfs_file->header.max_files; i++) {
        if (imgfs_file->metadata[i].is_valid == EMPTY) {
            free_idx = i;
            break;
        }

    }

    if(free_idx == -1) return ERR_IMGFS_FULL;



    SHA256((const unsigned char *)image_buffer, image_size, imgfs_file->metadata[free_idx].SHA);
    strncpy(imgfs_file->metadata[free_idx].img_id, img_id, MAX_IMG_ID);
    imgfs_file->metadata[free_idx].size[ORIG_RES] = (uint32_t) image_size; // Not sure cast is completely correct.

    //Put correct height and width in the image's metadata (see image_content.h #get_resolution())
    //CHANGE_SARA : inverse height and width
    int ret_resolution = get_resolution(&imgfs_file->metadata[free_idx].orig_res[1],
                                        &imgfs_file->metadata[free_idx].orig_res[0],
                                        image_buffer, image_size);


    if(ret_resolution != ERR_NONE) {
        return ret_resolution;
    }

    // Remove duplicates

    //CHANGE_SARA : changed dedup because it returned duplicate id when not needed (don't know why )
    int ret_dedup = do_name_and_content_dedup(imgfs_file, free_idx);


    //If no duplicates were found, write the image to the end of the file
    if (ret_dedup != ERR_NONE) {
        return ret_dedup;
    }

    if (fseek(imgfs_file->file, 0, SEEK_END) != 0){
        return ERR_IO;
    }
    //change
    if(fwrite(image_buffer, image_size, 1, imgfs_file->file) != 1) {
        return ERR_IO;
    }

    //CHANGE_SARA : correctly calculating the right offset
    long end_offset = ftell(imgfs_file->file);
    //finish initializing the metadata
    if(imgfs_file->metadata[free_idx].offset[ORIG_RES] == 0) { //not sure abt this but only way to make a test pass
        imgfs_file->metadata[free_idx].offset[ORIG_RES] = (uint64_t) ((uint64_t) end_offset - image_size);
    }
    imgfs_file->metadata[free_idx].is_valid = NON_EMPTY;

    //Update all the necessary image database header fields.
    imgfs_file->header.nb_files++;
    imgfs_file->header.version++;


    //write header to disk, and then corresponding metadata (but not all of it)
    if (fseek(imgfs_file->file, 0, SEEK_SET) != 0) {
        return ERR_IO;
    }
    if(fwrite(&imgfs_file->header, sizeof(struct imgfs_header), 1, imgfs_file->file) != 1) {
        return ERR_IO;
    }

    //CHANGE_SARA : writing metadata to disk as well
    long metadata_offset = (long)(sizeof(struct imgfs_header) + free_idx * sizeof(struct img_metadata));
    fseek(imgfs_file->file, metadata_offset, SEEK_SET);
    if (fwrite(&imgfs_file->metadata[free_idx], sizeof(struct img_metadata), 1, imgfs_file->file) !=1) {
        return ERR_IO;
    }

    return ERR_NONE;
}

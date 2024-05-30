#include "imgfs.h"
#include <string.h> // for strncpy
#include "image_dedup.h" // for do_name_and_content_dedup()
#include "image_content.h" // for get_resolution()


int do_insert(const char *image_buffer, size_t image_size,
              const char *img_id, struct imgfs_file *imgfs_file)
{

    //Arguments validity check
    M_REQUIRE_NON_NULL(img_id);
    M_REQUIRE_NON_NULL(image_buffer);
    M_REQUIRE_NON_NULL(imgfs_file);

    //Image size validity check
    if (image_size == 0) {
        return ERR_INVALID_ARGUMENT;
    }

    //Image full check
    if(imgfs_file->header.nb_files >= imgfs_file->header.max_files) {
        return ERR_IMGFS_FULL;
    }

    //Find empty entry index in metadata table
    int free_idx = -1;
    for (int i = 0; i < imgfs_file->header.max_files; i++) {
        if (imgfs_file->metadata[i].is_valid == EMPTY) {
            free_idx = i;
            break;
        }
    }

    if(free_idx == -1) return ERR_IMGFS_FULL;

    //Placing the image's SHA256 hash value in the SHA field
    SHA256((const unsigned char *)image_buffer, image_size, imgfs_file->metadata[free_idx].SHA);
    //Copying the img_id into the corresponding field
    strncpy(imgfs_file->metadata[free_idx].img_id, img_id, MAX_IMG_ID);
    //Storing the image size
    imgfs_file->metadata[free_idx].size[ORIG_RES] = (uint32_t) image_size;

    //Putting correct height and width in the image's metadata
    int ret_resolution = get_resolution(&imgfs_file->metadata[free_idx].orig_res[1],
                                        &imgfs_file->metadata[free_idx].orig_res[0],
                                        image_buffer, image_size);


    //In case of error in get_resolution() returning it
    if(ret_resolution != ERR_NONE) {
        return ret_resolution;
    }

    // Removing duplicates
    int ret_dedup = do_name_and_content_dedup(imgfs_file, free_idx);

    if (ret_dedup != ERR_NONE) {
        return ret_dedup;
    }

    //If no duplicates were found, we write the image to the end of the file
    if(imgfs_file->metadata[free_idx].offset[ORIG_RES] == 0) {

        if (fseek(imgfs_file->file, 0, SEEK_END) != 0) {
            return ERR_IO;
        }

        if(fwrite(image_buffer, image_size, 1, imgfs_file->file) != 1) {
            return ERR_IO;
        }

        //Calculating the offset of the new image
        long end_offset = ftell(imgfs_file->file);
        imgfs_file->metadata[free_idx].offset[ORIG_RES] = (uint64_t) ((uint64_t) end_offset - image_size);

        //finish initializing the metadata for other resolutions
        for (int resolution = THUMB_RES; resolution < (NB_RES-1); resolution++) {
            imgfs_file->metadata[free_idx].size[resolution] = 0;
            imgfs_file->metadata[free_idx].offset[resolution] = 0;
        }
    }

    //Updating the validity of the new image
    imgfs_file->metadata[free_idx].is_valid = NON_EMPTY;

    //Updating all the necessary image database header fields.
    imgfs_file->header.nb_files++;
    imgfs_file->header.version++;


    //Writing header to disk, and then corresponding metadata (but not all of it)
    if (fseek(imgfs_file->file, 0, SEEK_SET) != 0) {
        return ERR_IO;
    }
    if(fwrite(&imgfs_file->header, sizeof(struct imgfs_header), 1, imgfs_file->file) != 1) {
        return ERR_IO;
    }

    long metadata_offset = (long)(sizeof(struct imgfs_header) + free_idx * sizeof(struct img_metadata));

    if (fseek(imgfs_file->file, metadata_offset, SEEK_SET) != 0) {
        return ERR_IO;
    }

    if (fwrite(&imgfs_file->metadata[free_idx], sizeof(struct img_metadata), 1, imgfs_file->file) !=1) {
        return ERR_IO;
    }

    return ERR_NONE;
}

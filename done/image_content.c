#include "imgfs.h"
#include <string.h> // for strncpy
#include <stdlib.h> // for calloc
#include "image_content.h"
#include <vips/vips.h>

int lazily_resize(int resolution, struct imgfs_file* imgfs_file, size_t index){

    if (resolution != THUMB_RES && resolution != SMALL_RES) {
        return ERR_INVALID_ARGUMENT; // Return appropriate error value
    }

    if (imgfs_file == NULL || index >= imgfs_file->header.nb_files) {
        return ERR_INVALID_ARGUMENT; // Invalid file pointer
    }
    //if the image is already with the right resolution
    if(*imgfs_file->metadata[index].orig_res == (uint32_t)resolution) {
        return ERR_NONE;
    }
    int target_size;
    if (resolution == THUMB_RES) {
        target_size = imgfs_file->header.resized_res[0];
    } else  {
        target_size = imgfs_file->header.resized_res[2];
    }

    uint32_t orig_res = imgfs_file->metadata[index].orig_res[0];

    //creating a vips image of the imgfs file
    //TODO : test if should be size[0] or orig_res
    uint64_t offset = imgfs_file->metadata[index].offset[orig_res]; // Assuming you want to read the original resolution

    // Seek to the offset of the image data within the file
    fseek(imgfs_file->file, (long)offset, SEEK_SET);
    size_t img_size = imgfs_file->metadata[index].size[orig_res];
    unsigned char *img_data = calloc(img_size, sizeof(unsigned char));
    if (!img_data) {
        // Handle memory allocation failure
        return ERR_OUT_OF_MEMORY;
    }
    fread(img_data, 1, img_size, imgfs_file->file);

    // Create a Vips image from the file
    VipsImage *orig_image ;
    if( vips_jpegload_buffer( img_data, img_size, &orig_image, NULL ) ) {
        vips_error_exit("unable to decode jpeg");
    }




    VipsImage* resized_image = NULL;
    if (vips_thumbnail_image(orig_image, &resized_image, target_size, NULL) != 0) {
        // Handle resizing failure
        g_object_unref(orig_image);
        free(img_data);
        return ERR_IMGLIB;
    }
    void *buffer;
    size_t buffer_size;
    if (vips_jpegsave_buffer(resized_image, &buffer, &buffer_size, NULL) != 0) {
        // Handle saving failure
        g_object_unref(orig_image);
        g_object_unref(resized_image);
        free(img_data);
        return ERR_IMGLIB;
    }

    // Write the buffer contents to the end of the imgFS file
    fwrite(buffer, 1, buffer_size, imgfs_file->file);

    // Update metadata for the new image
    imgfs_file->metadata[index].offset[resolution] = ftell(imgfs_file->file) - buffer_size;
    imgfs_file->metadata[index].size[resolution] = buffer_size;
    imgfs_file->metadata[index].is_valid = 1; // Mark the image as valid

    // Cleanup
    g_object_unref(orig_image);
    g_object_unref(resized_image);
    free(img_data);
    g_free(buffer);



    return ERR_NONE;
}

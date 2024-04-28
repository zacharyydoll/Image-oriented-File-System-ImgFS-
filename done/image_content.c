#include "imgfs.h"
#include <string.h> // for strncpy
#include <stdlib.h> // for calloc
#include "image_content.h"
#include <vips/vips.h>

int lazily_resize(int resolution, struct imgfs_file* imgfs_file, size_t index){
    printf("currently in lazy resize function");
    M_REQUIRE_NON_NULL(imgfs_file);


    if (resolution != THUMB_RES && resolution != SMALL_RES && resolution != ORIG_RES) {
        return ERR_INVALID_ARGUMENT; // Return appropriate error value
    }
    if (index > imgfs_file->header.nb_files) {
        return ERR_INVALID_IMGID; // Invalid file pointer
    }
    //if the image is already with the right resolution
    if(imgfs_file->metadata[index].orig_res[resolution] == (uint32_t)resolution) {
        return ERR_NONE;
    }
    uint32_t target_height;
    uint32_t target_width;
    if (resolution == THUMB_RES) {
        // Use thumbnail width and height from imgfs_header
        target_width = imgfs_file->header.resized_res[0]; // Thumbnail width
        target_height = imgfs_file->header.resized_res[1]; // Thumbnail height
    } else if (resolution == SMALL_RES) {
        // Use small image width and height from imgfs_header
        target_width = imgfs_file->header.resized_res[2]; // Small image width
        target_height = imgfs_file->header.resized_res[3]; // Small image height
    } else {
        // Keep original width and height for original resolution
        target_width = imgfs_file->metadata->orig_res[0];
        target_height = imgfs_file->metadata->orig_res[1];
    }
    //creating a vips image of the imgfs file
    uint64_t offset = imgfs_file->metadata[index].offset[ORIG_RES]; // reading the original resolution

    // Seek to the offset of the image data within the file
    size_t img_size = imgfs_file->metadata[index].size[ORIG_RES];
    unsigned char *img_data = calloc(img_size, sizeof(unsigned char));
    if (!img_data) {
        // Handle memory allocation failure
        return ERR_OUT_OF_MEMORY;
    }
    // Seek to the offset of the image data within the file
    if (fseek(imgfs_file->file, (long)offset, SEEK_SET) != 0) {
        // Handle fseek failure
        free(img_data);
        return ERR_IO;
    }

    size_t bytes_read = fread(img_data, 1, img_size, imgfs_file->file);
    if (bytes_read != imgfs_file->metadata[index].size[ORIG_RES]) {
        // Handle fread failure or incomplete read
        free(img_data);
        return ERR_IO;
    }

    // Create a Vips image from the file
    VipsImage *orig_image ;
    if( vips_jpegload_buffer( img_data, img_size, &orig_image, NULL ) ) {
        //vips_error_exit("unable to decode jpeg");
        return ERR_IO;
    }




    VipsImage* resized_image = NULL;
    if (vips_thumbnail_image(orig_image, &resized_image, target_width, "height", target_height, NULL) != 0) {
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
    if (fwrite(buffer, 1, buffer_size, imgfs_file->file) != buffer_size) {
        // Handle fwrite failure or incomplete write
        g_object_unref(orig_image);
        g_object_unref(resized_image);
        free(img_data);
        g_free(buffer);
        return ERR_IO;
    }



    // Update metadata for the new image
    long end_offset = ftell(imgfs_file->file);
    imgfs_file->metadata[index].offset[resolution] = end_offset - buffer_size;
    imgfs_file->metadata[index].size[resolution] = buffer_size;
    imgfs_file->metadata[index].is_valid = 1; // Mark the image as valid

    // Cleanup
    g_object_unref(orig_image);
    g_object_unref(resized_image);
    free(img_data);
    g_free(buffer);



    return ERR_NONE;
}

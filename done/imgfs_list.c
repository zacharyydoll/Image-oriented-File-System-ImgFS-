#include "imgfs.h"  // for print_header, print_metadata
#include "util.h"   // for TO_BE_IMPLEMENTED()
#include "error.h"  // for ERR_NONE
#include "stdio.h"  // for print
#include <json-c/json.h> // for JSON functions
#include <string.h>


int do_list(const struct imgfs_file *imgfs_file,
            enum do_list_mode output_mode, char **json)
{
    //Argument validity check
    M_REQUIRE_NON_NULL(imgfs_file);


    // If the output mode is STDOUT, print the header and metadata, or indicate if the imgFS is empty.
    if (output_mode == STDOUT) {
        print_header(&imgfs_file->header);

        if (imgfs_file->header.nb_files == 0) { //No files
            printf("<< empty imgFS >>\n");
        } else {
            // Iterate over the metadata array
            for (uint32_t i = 0; i < imgfs_file->header.max_files; ++i) {
                // If the metadata slot is filled (NON_EMPTY), print the metadata.
                if (imgfs_file->metadata[i].is_valid == NON_EMPTY) {
                    print_metadata(&(imgfs_file->metadata[i]));
                }
            }
        }

        return ERR_NONE;

    } else if (output_mode == JSON) {

        // WEEK 13 ====================================================================================================

        //create JSON object to hold array of image IDs
        struct json_object *json_obj = json_object_new_object();
        if (!json_obj) {
            return ERR_RUNTIME;
        }

        struct json_object *json_arr = json_object_new_array();
        if (!json_arr) {
            json_object_put(json_obj); // free (handout)
            return ERR_RUNTIME;
        }

        //iterate over metadata array and add each valid image ID to the JSON array
        for (uint32_t i = 0; i < imgfs_file->header.max_files; i++) {
            if (imgfs_file->metadata[i].is_valid == NON_EMPTY) {
                struct json_object *json_str = json_object_new_string(imgfs_file->metadata[i].img_id);
                if (!json_str) {
                    json_object_put(json_arr);
                    json_object_put(json_obj);
                    return ERR_RUNTIME;
                }
                json_object_array_add(json_arr, json_str);
            }
        }

        // Add array to the JSON object
        json_object_object_add(json_obj, "Images", json_arr); // (see handout tests for key)

        //convert JSON object to string
        const char *json_str = json_object_to_json_string(json_obj);
        if (!json_str) {
            json_object_put(json_obj);
            return ERR_RUNTIME;
        }

        // allocate memory for JSON string and copy it
        *json = strdup(json_str); // strdup cleaner than strcpy -> no need for another malloc
        if (!*json) {
            json_object_put(json_obj); //failed to cpy
            return ERR_RUNTIME;
        }

        json_object_put(json_obj); // free

        return ERR_NONE;

        // ============================================================================================================

    } else {
        //case where output mode is undefined
        debug_printf("Unknown output mode: %d\n", output_mode);
        return ERR_INVALID_ARGUMENT;
    }
}

#include "imgfs_tools.c" // for print_header() method (???) Is there no header file for imgfs_tools ?
#include "util.h" // for TO_BE_IMPLEMENTED()
#include "stdio.h" // for print

int do_list(const struct imgfs_header *header, const struct img_metadata *metadata, int output_mode) {
    // argument validity check
    M_REQUIRE_NON_NULL(header);
    M_REQUIRE_NON_NULL(metadata);

    if (output_mode == STDOUT) {
        print_header(header);

        int isEmpty = 1; // Array assumed to be empty until we find a valid entry
        for (int i = 0; i < header->max_files; i++) {
            // if we have a valid entry, print it and set empty flag to false
            if (metadata[i].is_valid) {
                isEmpty = 0;
                print_metadata(&metadata[i]);
            }
        }

        if (isEmpty) {
            printf("<< empty imgFS >>"); // database doesn't contain any images
        }

        return ERR_NONE;
    } else if (output_mode == JSON) {
        TO_BE_IMPLEMENTED(); // will be implemented later on in project
        return ERR_NOT_IMPLEMENTED;
    } else {
        //case where output mode is undefined
        debug_printf("Unknown output mode: %d\n", output_mode);
        return ERR_INVALID_ARGUMENT;
    }
}
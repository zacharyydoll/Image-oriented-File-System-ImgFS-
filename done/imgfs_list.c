#include "imgfs.h"  // for print_header, print_metadata
#include "util.h"   // for TO_BE_IMPLEMENTED()
#include "error.h"  // for ERR_NONE
#include "stdio.h"  // for print


int do_list(const struct imgfs_file *imgfs_file,
            enum do_list_mode output_mode, char **json) {
    //argument validity check
    M_REQUIRE_NON_NULL(imgfs_file);
    //M_REQUIRE_NON_NULL(imgfs_file->metadata);

    //CHANGE_SARA : the code was reading from metadata even if empty changed the function
    if (output_mode == STDOUT) {
        print_header(&imgfs_file->header);

        if (imgfs_file->header.nb_files == 0) {
            printf("<< empty imgFS >>\n");
        } else{
            //CHANGE_SARA : we should parcours all files from disk
            for (uint32_t i = 0; i < imgfs_file->header.max_files; ++i) {
                if (imgfs_file->metadata[i].is_valid == NON_EMPTY) {
                    print_metadata(&(imgfs_file->metadata[i]));
                }
            }
        }

        return ERR_NONE;
    } else if (output_mode == JSON) {
        //TODO : use json argument in future implementation
        TO_BE_IMPLEMENTED();
        return NOT_IMPLEMENTED;
    } else {
        //case where output mode is undefined
        debug_printf("Unknown output mode: %d\n", output_mode);
        return ERR_INVALID_ARGUMENT;;
    }
}
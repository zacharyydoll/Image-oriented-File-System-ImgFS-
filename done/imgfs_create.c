#include "imgfs.h"
#include <string.h> // for strncpy
#include <stdlib.h> // for calloc

int do_create(const char *imgfs_filename, struct imgfs_file *imgfs_file)
{
    // Parameter validity check
    M_REQUIRE_NON_NULL(imgfs_filename);
    M_REQUIRE_NON_NULL(imgfs_file);


    // Opening the file
    imgfs_file->file = fopen(imgfs_filename, "wb");
    if (!(imgfs_file->file)) {
        return ERR_IO;
    }

    // Finishing initialization of the header
    strncpy(imgfs_file->header.name, CAT_TXT, sizeof(imgfs_file->header.name) - 1);
    imgfs_file->header.name[sizeof(imgfs_file->header.name) - 1] = '\0';
    imgfs_file->header.version = 0;
    imgfs_file->header.nb_files = 0; // no files yet
    imgfs_file->header.unused_32 = 0;
    imgfs_file->header.unused_64 = 0;

    // Initializing all bytes of metadata to 0
    imgfs_file->metadata = calloc(imgfs_file->header.max_files, sizeof(struct img_metadata));
    if (imgfs_file->metadata == NULL) {
        fclose(imgfs_file->file);
        return ERR_OUT_OF_MEMORY;
    }

    // Handling writing errors in the header
    if (fwrite(&(imgfs_file->header), sizeof(struct imgfs_header), 1, imgfs_file->file) != 1) {
        fclose(imgfs_file->file);
        free(imgfs_file->metadata); // Free the memory allocated before returning
        return ERR_IO;
    }
    // Handling write errors in the metadata
    if (fwrite(imgfs_file->metadata,
               sizeof(struct img_metadata),
               imgfs_file->header.max_files,
               imgfs_file->file) != imgfs_file->header.max_files) {

        fclose(imgfs_file->file);
        free(imgfs_file->metadata);
        return ERR_IO;
    }
    // print # of items written
    printf("%d item(s) written\n", 1 + imgfs_file->header.nb_files); // header + each metadata entry

    return ERR_NONE;
}

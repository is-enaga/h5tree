// main.c
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <hdf5.h>
#include "utils.h"
#include "tree.h"
#include "json.h"

// treeh5.c 側で提供している関数
// extern herr_t group_iter(hid_t loc_id, const char *name,
//                          const H5L_info_t *linfo, void *op_data);

int main(int argc, char **argv) {
    int json_mode = 0;
    use_color = isatty(STDERR_FILENO);
    show_structure_only = 0;
    const char *filename = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--json") == 0) {
            json_mode = 1;
        } else if (strcmp(argv[i], "--structure") == 0 || strcmp(argv[i], "-s") == 0) {
            show_structure_only = 1;
        } else {
            filename = argv[i];
        }
    }

    if (!filename) {
        fprintf(stderr, "Usage: %s file.h5 [--json] [--structure-only]\n", argv[0]);
        return 1;
    }

    hid_t file = H5Fopen(filename, H5F_ACC_RDONLY, H5P_DEFAULT);
    if (file < 0) {
        fprintf(stderr, "Failed to open %s\n", filename);
        return 1;
    }

    if (json_mode) {
        print_hdf5_as_json(file);
    } else {
        IterData idata = {0};
        group_iter(file, "/", NULL, &idata);
    }

    H5Fclose(file);
    return 0;
}
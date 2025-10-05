// main.c
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <hdf5.h>
#include "utils.h"
#include "tree.h"
#include "json.h"


int main(int argc, char **argv) {
    int json_mode = 0;
    use_color = isatty(STDERR_FILENO);
    show_structure_only = 0;
    const char *filename = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--version") == 0) {
            printf("%s %s (c) 2025 by Akikaze\n", argv[0], VERSION);
            return 0;
        }
        if (strcmp(argv[i], "--json") == 0 || strcmp(argv[i], "-j") == 0) {
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
        // fprintf(stderr, "%s %s (c) 2025 by Akikaze\n", argv[0], VERSION);
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
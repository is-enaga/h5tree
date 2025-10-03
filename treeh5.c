// treeh5.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>   // isatty, STDOUT_FILENO
#include <hdf5.h>

#define MAX_DEPTH 64

typedef struct {
    int depth;
    int last_flags[MAX_DEPTH]; // その階層で最後の要素なら1
} IterData;

// 色制御
static int use_color = 0;
#define COLOR_RESET   (use_color ? "\033[0m"  : "")
#define COLOR_GROUP   ""                        // グループは白（色なし）
#define COLOR_DATASET (use_color ? "\033[92m" : "")  // 明るい緑
#define COLOR_ATTR    (use_color ? "\033[96m" : "")  // 明るい水色

static void print_indent(int depth, const int *last_flags) {
    for (int i = 1; i < depth; i++) {
        printf("%s   ", last_flags[i] ? "    " : "│");
    }
    if (depth > 0) {
        printf("%s── ", last_flags[depth] ? "└" : "├");
    }
}

static void show_attrs(hid_t obj, IterData *idata, hsize_t siblings_total, hsize_t siblings_index_base) {
    hsize_t nattrs = (hsize_t)H5Aget_num_attrs(obj);
    for (hsize_t j = 0; j < nattrs; j++) {
        IterData attrdata = *idata;
        attrdata.depth++;

        hsize_t global_idx = siblings_index_base + j;
        attrdata.last_flags[attrdata.depth] = (global_idx == siblings_total - 1);

        hid_t attr = H5Aopen_by_idx(obj, ".", H5_INDEX_NAME, H5_ITER_NATIVE,
                                    j, H5P_DEFAULT, H5P_DEFAULT);
        if (attr >= 0) {
            char aname[256];
            ssize_t len = H5Aget_name(attr, sizeof(aname), aname);
            if (len >= 0) {
                print_indent(attrdata.depth, attrdata.last_flags);
                // 属性は明るい水色
                printf("%s%s%s\n", COLOR_ATTR, aname, COLOR_RESET);
            }
            H5Aclose(attr);
        }
    }
}

static herr_t group_iter(hid_t loc_id, const char *name,
                         const H5L_info_t *linfo, void *op_data) {
    (void)linfo;

    IterData *idata = (IterData *)op_data;

    hid_t obj = H5Oopen(loc_id, name, H5P_DEFAULT);
    if (obj < 0) return 0;

    H5O_info_t oinfo;
    H5Oget_info(obj, &oinfo);

    print_indent(idata->depth, idata->last_flags);

    if (oinfo.type == H5O_TYPE_GROUP) {
        printf("%s%s%s\n", COLOR_GROUP, name, COLOR_RESET);

        hsize_t nlinks = 0;
        H5Gget_num_objs(obj, &nlinks);
        hsize_t nattrs = (hsize_t)H5Aget_num_attrs(obj);
        hsize_t total = nattrs + nlinks;

        show_attrs(obj, idata, total, 0);

        for (hsize_t i = 0; i < nlinks; i++) {
            char childname[1024];
            ssize_t len = H5Lget_name_by_idx(obj, ".", H5_INDEX_NAME, H5_ITER_NATIVE,
                                             i, childname, sizeof(childname), H5P_DEFAULT);
            if (len < 0) continue;

            IterData childdata = *idata;
            childdata.depth++;
            childdata.last_flags[childdata.depth] = ((nattrs + i) == total - 1);

            group_iter(obj, childname, NULL, &childdata);
        }

    } else if (oinfo.type == H5O_TYPE_DATASET) {
        printf("%s%s%s\n", COLOR_DATASET, name, COLOR_RESET);

        hsize_t nattrs = (hsize_t)H5Aget_num_attrs(obj);
        hsize_t total = nattrs;

        show_attrs(obj, idata, total, 0);

    } else {
        printf("%s\n", name);
    }

    H5Oclose(obj);
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s file.h5\n", argv[0]);
        return 1;
    }

    hid_t file = H5Fopen(argv[1], H5F_ACC_RDONLY, H5P_DEFAULT);
    if (file < 0) {
        fprintf(stderr, "Cannot open file: %s\n", argv[1]);
        return 1;
    }

    // 出力先が端末なら色を使う
    use_color = isatty(STDOUT_FILENO);

    printf("%s\n", argv[1]);

    hid_t root = H5Gopen(file, "/", H5P_DEFAULT);
    if (root < 0) {
        fprintf(stderr, "Cannot open root group\n");
        H5Fclose(file);
        return 1;
    }

    hsize_t root_nlinks = 0;
    H5Gget_num_objs(root, &root_nlinks);
    hsize_t root_nattrs = (hsize_t)H5Aget_num_attrs(root);
    hsize_t root_total = root_nattrs + root_nlinks;

    IterData rootdata = {0};
    rootdata.depth = 0;
    memset(rootdata.last_flags, 0, sizeof(rootdata.last_flags));

    show_attrs(root, &rootdata, root_total, 0);

    for (hsize_t i = 0; i < root_nlinks; i++) {
        char childname[1024];
        ssize_t len = H5Lget_name_by_idx(root, ".", H5_INDEX_NAME, H5_ITER_NATIVE,
                                         i, childname, sizeof(childname), H5P_DEFAULT);
        if (len < 0) continue;

        IterData childdata = {0};
        childdata.depth = 1;
        memset(childdata.last_flags, 0, sizeof(childdata.last_flags));
        childdata.last_flags[childdata.depth] = ((root_nattrs + i) == root_total - 1);

        group_iter(root, childname, NULL, &childdata);
    }

    H5Gclose(root);
    H5Fclose(file);
    return 0;
}
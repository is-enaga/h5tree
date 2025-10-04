#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>   // isatty
#include <hdf5.h>

#include "utils.h"
#include "tree.h"

#define MAX_DEPTH 64

int use_color = 0;
int show_structure_only = 0;

static void print_indent(int depth, const int *last_flags) {
    for (int i = 1; i < depth; i++) {
        printf("%s   ", last_flags[i] ? "    " : "│");
    }
    if (depth > 0) {
        printf("%s── ", last_flags[depth] ? "└" : "├");
    }
}



void print_dataset_info(hid_t dset) {
    hid_t space = H5Dget_space(dset);
    int ndims = H5Sget_simple_extent_ndims(space);
    hsize_t dims[32] = {0};
    H5Sget_simple_extent_dims(space, dims, NULL);

    hid_t dtype = H5Dget_type(dset);
    char type_name[32] = "unknown";
    H5T_class_t cls = H5Tget_class(dtype);
    size_t size = H5Tget_size(dtype);
    if (cls == H5T_INTEGER) {
        H5T_sign_t sign = H5Tget_sign(dtype);
        snprintf(type_name, sizeof(type_name),
                 "%s%zu", (sign == H5T_SGN_NONE ? "uint" : "int"), size * 8);
    } else if (cls == H5T_FLOAT) {
        snprintf(type_name, sizeof(type_name), "float%zu", size * 8);
    } else if (cls == H5T_STRING) {
        snprintf(type_name, sizeof(type_name), "string");
    }

    hid_t dcpl = H5Dget_create_plist(dset);
    hsize_t storage_size = H5Dget_storage_size(dset);
    char size_str[32];
    format_size(size_str, sizeof(size_str), storage_size);

    int nfilters = H5Pget_nfilters(dcpl);
    char filter_info[128] = "";
    for (int i = 0; i < nfilters; i++) {
        unsigned int flags;
        size_t cd_nelmts = 20;
        unsigned int cd_values[20];
        char name[64];
        H5Z_filter_t filter = H5Pget_filter(dcpl, i, &flags, &cd_nelmts, cd_values, sizeof(name), name, NULL);
        if (filter == H5Z_FILTER_DEFLATE) {
            unsigned int level = (cd_nelmts > 0) ? cd_values[0] : 0;
            char size_str_comp[32];
            format_size(size_str_comp, sizeof(size_str_comp), storage_size);

            snprintf(filter_info, sizeof(filter_info),
                    "(%s%s level=%u%s)=>%s", COLOR_COMP, "gzip", level, COLOR_RESET, size_str_comp);
        } else if (filter == H5Z_FILTER_SZIP) {
            unsigned int options_mask = (cd_nelmts > 0) ? cd_values[0] : 0;
            unsigned int pixels_per_block = (cd_nelmts > 1) ? cd_values[1] : 0;
            snprintf(filter_info, sizeof(filter_info),
                    "(%s%s%s options=0x%X pixels_per_block=%u)", COLOR_COMP, "szip", COLOR_RESET,
                    options_mask, pixels_per_block);
        } else if (filter == H5Z_FILTER_SHUFFLE) {
            snprintf(filter_info, sizeof(filter_info),
                    "(%s%s%s)", COLOR_COMP, "shuffle", COLOR_RESET);
        } else if (filter == H5Z_FILTER_FLETCHER32) {
            snprintf(filter_info, sizeof(filter_info),
                    "(%s%s%s)", COLOR_COMP, "fletcher32", COLOR_RESET);
        } else if (filter == H5Z_FILTER_NBIT) {
            snprintf(filter_info, sizeof(filter_info),
                    "(%s%s%s)", COLOR_COMP, "nbit", COLOR_RESET);
        } else if (filter == H5Z_FILTER_SCALEOFFSET) {
            snprintf(filter_info, sizeof(filter_info),
                    "(%s%s%s)", COLOR_COMP, "scaleoffset", COLOR_RESET);
        } else {
            snprintf(filter_info, sizeof(filter_info),
                    "(%sunknown filter id=%d%s)", COLOR_COMP, (int)filter, COLOR_RESET);
        }
    }
    printf("\t[shape=(");
    for (int i = 0; i < ndims; i++) {
        printf("%s%llu", i ? "," : "", (unsigned long long)dims[i]);
    }

    hsize_t logical_size = size; // 型サイズ（バイト）
    for (int i = 0; i < ndims; i++) {
        logical_size *= dims[i];
    }
    char logical_str[32], compressed_str[32];
    format_size(logical_str, sizeof(logical_str), logical_size);
    format_size(compressed_str, sizeof(compressed_str), storage_size);

    printf(")/%s%s%s/%s", COLOR_TYPE, type_name, COLOR_RESET, logical_str);
    if (filter_info[0]) {
        printf("%s", filter_info); // 例: (gzip level=6 -> 1.2KB)
    }
    printf("]");

    H5Sclose(space);
    H5Tclose(dtype);
    H5Pclose(dcpl);
}

void print_attr_value(hid_t attr) {
    hid_t type = H5Aget_type(attr);
    H5T_class_t cls = H5Tget_class(type);

    printf("\t= ");
    if (cls == H5T_INTEGER) {
        int val = 0;
        H5Aread(attr, type, &val);
        H5T_sign_t sign = H5Tget_sign(type);
        size_t size = H5Tget_size(type);
        printf("%d (", val);
        if (sign == H5T_SGN_NONE)
            printf(" (%suint%zu%s)", COLOR_TYPE, size * 8, COLOR_RESET);
        else
            printf(" (%sint%zu%s)", COLOR_TYPE, size * 8, COLOR_RESET);
        printf(")");
    }
    else if (cls == H5T_FLOAT) {
        double val = 0;
        H5Aread(attr, type, &val);
        size_t size = H5Tget_size(type);
        printf(" (%sfloat%zu%s)", COLOR_TYPE, size * 8, COLOR_RESET);
    } else if (cls == H5T_STRING) {
        if (H5Tis_variable_str(type)) {
            char *vstr = NULL;
            H5Aread(attr, type, &vstr);
            if (vstr) {
                printf("\"%s\" (%sstring%s)", vstr, COLOR_TYPE, COLOR_RESET);
                free(vstr);
            }
        } else {
            char buf[256] = "";
            H5Aread(attr, type, buf);
            printf(" (%sstring%s)", COLOR_TYPE, COLOR_RESET);
        }
    }
    H5Tclose(type);
}

void show_attrs(hid_t obj, IterData *idata, hsize_t siblings_total, hsize_t siblings_index_base) {
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
                printf("%s%s%s", COLOR_ATTR, aname, COLOR_RESET);
                if (!show_structure_only) {
                    print_attr_value(attr);
                }
                printf("\n");
            }
            H5Aclose(attr);
        }
    }
}

herr_t group_iter(hid_t loc_id, const char *name,
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
        printf("%s%s%s", COLOR_DATASET, name, COLOR_RESET);
        if (!show_structure_only) {
            print_dataset_info(obj);
        }
        printf("\n");

        hsize_t nattrs = (hsize_t)H5Aget_num_attrs(obj);
        show_attrs(obj, idata, nattrs, 0);

    } else {
        printf("%s\n", name);
    }

    H5Oclose(obj);
    return 0;
}


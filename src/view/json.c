#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hdf5.h>

#include "utils.h"
#include "json.h"

#define MAX_DEPTH 64

// JSON 出力用のインデント
static void print_indent(int depth) {
    for (int i = 0; i < depth; i++) {
        printf("  "); // 2スペース
    }
}

// データセット情報を JSON に出力
static void json_print_dataset_info(hid_t dset, int depth) {
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

    // 容量計算
    hsize_t logical_size = size;
    for (int i = 0; i < ndims; i++) {
        logical_size *= dims[i];
    }
    hsize_t storage_size = H5Dget_storage_size(dset);

    print_indent(depth);
    printf("\"dtype\": \"%s\",\n", type_name);

    print_indent(depth);
    printf("\"shape\": [");
    for (int i = 0; i < ndims; i++) {
        printf("%s%llu", (i ? ", " : ""), (unsigned long long)dims[i]);
    }
    printf("],\n");

    print_indent(depth);
    printf("\"logical_size\": %llu,\n", (unsigned long long)logical_size);

    print_indent(depth);
    printf("\"storage_size\": %llu", (unsigned long long)storage_size);

    H5Sclose(space);
    H5Tclose(dtype);
}

// 属性を JSON に出力
static void json_show_attrs(hid_t obj, int depth) {
    hsize_t nattrs = (hsize_t)H5Aget_num_attrs(obj);
    if (nattrs == 0) return;

    print_indent(depth);
    printf("\"attributes\": {\n");

    for (hsize_t j = 0; j < nattrs; j++) {
        hid_t attr = H5Aopen_by_idx(obj, ".", H5_INDEX_NAME, H5_ITER_NATIVE,
                                    j, H5P_DEFAULT, H5P_DEFAULT);
        if (attr >= 0) {
            char aname[256];
            ssize_t len = H5Aget_name(attr, sizeof(aname), aname);
            if (len >= 0) {
                print_indent(depth + 1);
                printf("\"%s\": ", aname);

                hid_t type = H5Aget_type(attr);
                H5T_class_t cls = H5Tget_class(type);
                if (cls == H5T_INTEGER) {
                    int val = 0;
                    H5Aread(attr, type, &val);
                    printf("%d", val);
                } else if (cls == H5T_FLOAT) {
                    double val = 0;
                    H5Aread(attr, type, &val);
                    printf("%f", val);
                } else if (cls == H5T_STRING) {
                    if (H5Tis_variable_str(type)) {
                        char *vstr = NULL;
                        H5Aread(attr, type, &vstr);
                        if (vstr) {
                            printf("\"%s\"", vstr);
                            H5free_memory(vstr); // ← free() ではなく H5free_memory()
                        }
                    } else {
                        char buf[256] = "";
                        H5Aread(attr, type, buf);
                        printf("\"%s\"", buf);
                    }
                } else {
                    printf("\"<unsupported>\"");
                }
                H5Tclose(type);

                if (j < nattrs - 1) printf(",");
                printf("\n");
            }
            H5Aclose(attr);
        }
    }
    print_indent(depth);
    printf("}");
}

// 再帰的にグループ/データセットを JSON に出力
herr_t json_group_iter(hid_t loc_id, const char *name,
                       const H5L_info_t *linfo, void *op_data) {
    (void)linfo;
    int depth = *(int *)op_data;

    hid_t obj = H5Oopen(loc_id, name, H5P_DEFAULT);
    if (obj < 0) return 0;

    H5O_info_t oinfo;
    H5Oget_info(obj, &oinfo);

    print_indent(depth);
    printf("\"%s\": {\n", name);

    int inner_depth = depth + 1;
    int need_comma = 0;

    if (oinfo.type == H5O_TYPE_GROUP) {
        // 属性
        hsize_t nattrs = (hsize_t)H5Aget_num_attrs(obj);
        if (nattrs > 0) {
            json_show_attrs(obj, inner_depth);
            need_comma = 1;
        }

        // 子要素
        hsize_t nlinks = 0;
        H5Gget_num_objs(obj, &nlinks);
        if (nlinks > 0) {
            if (need_comma) printf(",\n");
            print_indent(inner_depth);
            printf("\"children\": {\n");
            for (hsize_t i = 0; i < nlinks; i++) {
                char childname[1024];
                ssize_t len = H5Lget_name_by_idx(obj, ".", H5_INDEX_NAME, H5_ITER_NATIVE,
                                                 i, childname, sizeof(childname), H5P_DEFAULT);
                if (len < 0) continue;
                int child_depth = inner_depth + 1;
                json_group_iter(obj, childname, NULL, &child_depth);
                if (i < nlinks - 1) printf(",");
                printf("\n");
            }
            print_indent(inner_depth);
            printf("}");
        }

    } else if (oinfo.type == H5O_TYPE_DATASET) {
        json_print_dataset_info(obj, inner_depth);
        hsize_t nattrs = (hsize_t)H5Aget_num_attrs(obj);
        if (nattrs > 0) {
            printf(",\n");
            json_show_attrs(obj, inner_depth);
        }
        printf("\n");
        print_indent(depth);
    }

    printf("}");

    H5Oclose(obj);
    return 0;
}

// エントリーポイント
void print_hdf5_as_json(hid_t file) {
    printf("{\n");
    int depth = 1;
    json_group_iter(file, "/", NULL, &depth);
    printf("\n}\n");
}
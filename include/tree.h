#ifndef TREE_H
#define TREE_H

#include <hdf5.h>

#define MAX_DEPTH 64

// ツリー走査時に使う状態
typedef struct {
    int depth;
    int last_flags[MAX_DEPTH];
} IterData;

// 外部から制御できるフラグ
extern int use_color;             // 色を出すかどうか
extern int show_structure_only;   // 構造だけ表示するか（属性や型情報を省略）

// 公開関数
void print_dataset_info(hid_t dset);
void print_attr_value(hid_t attr);
void show_attrs(hid_t obj, IterData *idata,
                hsize_t siblings_total, hsize_t siblings_index_base);
herr_t group_iter(hid_t loc_id, const char *name,
                  const H5L_info_t *linfo, void *op_data);

// 色マクロ（use_color に依存）
#define COLOR_RESET   (use_color ? "\033[0m"  : "")
#define COLOR_GROUP   (use_color ? "\033[96m" : "")
#define COLOR_DATASET (use_color ? "\033[92m" : "")
#define COLOR_ATTR    ""
#define COLOR_TYPE    (use_color ? "\033[96m" : "")
#define COLOR_COMP    (use_color ? "\033[91m" : "")

#endif // TREE_H
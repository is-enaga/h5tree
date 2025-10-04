#ifndef JSON_H
#define JSON_H

#include <hdf5.h>

// 外部から制御できるフラグ
extern int show_structure_only;   // 構造だけ表示するか（値や型情報を省略）

// 公開関数
void print_hdf5_as_json(hid_t file);

#endif
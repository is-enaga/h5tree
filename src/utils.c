#include <stdio.h>
#include <hdf5.h>
// #include "utils.h"

void format_size(char *buf, size_t bufsize, hsize_t bytes) {
    const char *units[] = {"B", "KB", "MB", "GB", "TB"};
    double size = (double)bytes;
    int unit = 0;
    while (size >= 1024.0 && unit < 4) {
        size /= 1024.0;
        unit++;
    }

    if (unit == 0) {
        // B のときは整数表示
        snprintf(buf, bufsize, "%llu%s", (unsigned long long)bytes, units[unit]);
    } else {
        // KB 以上は小数点付き
        snprintf(buf, bufsize, "%.1f%s", size, units[unit]);
    }
}
// utils.h
#ifndef UTILS_H
#define UTILS_H

#include <hdf5.h>
#include <stddef.h>

void format_size(char *buf, size_t bufsize, hsize_t bytes);

#endif
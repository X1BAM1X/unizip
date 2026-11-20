/* SPDX-License-Identifier: MIT */

#ifndef UNIZIP_H
#define UNIZIP_H

#include <stddef.h>

#define UZ_VERSION "0.1.0"
#define UZ_MAGIC_CAP 512

typedef enum {
    UZ_FORMAT_UNKNOWN = 0,
    UZ_FORMAT_ZIP,
    UZ_FORMAT_RAR,
    UZ_FORMAT_7Z,
    UZ_FORMAT_TAR,
    UZ_FORMAT_GZIP,
    UZ_FORMAT_BZIP2,
    UZ_FORMAT_XZ,
    UZ_FORMAT_ZSTD,
    UZ_FORMAT_LZ4,
    UZ_FORMAT_COMPRESS,
    UZ_FORMAT_LZIP,
    UZ_FORMAT_LZMA
} uz_format_t;

typedef enum {
    UZ_MODE_EXTRACT = 0,
    UZ_MODE_LIST,
    UZ_MODE_TEST
} uz_mode_t;

typedef enum {
    UZ_EXIT_OK = 0,
    UZ_EXIT_USAGE = 1,
    UZ_EXIT_FAILURE = 2,
    UZ_EXIT_TOOL_MISSING = 3,
    UZ_EXIT_INTERNAL = 4
} uz_exit_t;

typedef struct {
    uz_format_t format;
    int is_tar_wrapped;
    unsigned char magic[UZ_MAGIC_CAP];
    size_t magic_len;
} uz_archive_t;

const char *uz_format_name(uz_format_t fmt);

#endif

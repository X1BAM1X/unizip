/* SPDX-License-Identifier: MIT */

#include "detect.h"
#include "util.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int has_magic(const uz_archive_t *arc, const void *sig, size_t len)
{
    return arc->magic_len >= len && memcmp(arc->magic, sig, len) == 0;
}

static int looks_like_lzma(const uz_archive_t *arc)
{
    if (arc->magic_len < 5 || arc->magic[0] != 0x5d) {
        return 0;
    }

    uint32_t dict = (uint32_t)arc->magic[1]
                  | (uint32_t)arc->magic[2] << 8
                  | (uint32_t)arc->magic[3] << 16
                  | (uint32_t)arc->magic[4] << 24;

    return dict >= 4096 && (dict & 0xfff) == 0;
}

static int looks_like_tar(const char *path)
{
    const char *name = uz_basename(path);

    static const char *const suffixes[] = {
        ".tar.gz", ".tgz",  ".taz", ".tar.Z",
        ".tar.bz2", ".tbz", ".tbz2",
        ".tar.xz", ".txz",
        ".tar.zst", ".tar.zstd", ".tzst",
        ".tar.lz4", ".tlz4",
        ".tar.lz", ".tlz",
        ".tar.lzma",
        NULL
    };

    for (size_t i = 0; suffixes[i]; i++) {
        if (uz_ends_with(name, suffixes[i])) {
            return 1;
        }
    }
    return 0;
}

uz_format_t uz_detect_format(const char *path, uz_archive_t *out)
{
    if (!path || !out) {
        return UZ_FORMAT_UNKNOWN;
    }

    memset(out, 0, sizeof(*out));

    FILE *fp = fopen(path, "rb");
    if (!fp) {
        return UZ_FORMAT_UNKNOWN;
    }
    out->magic_len = fread(out->magic, 1, sizeof(out->magic), fp);
    fclose(fp);

    if (out->magic_len < 4) {
        out->format = UZ_FORMAT_UNKNOWN;
        return UZ_FORMAT_UNKNOWN;
    }

    if (has_magic(out, "PK\x03\x04", 4) ||
        has_magic(out, "PK\x05\x06", 4) ||
        has_magic(out, "PK\x07\x08", 4)) {
        out->format = UZ_FORMAT_ZIP;
    } else if (has_magic(out, "Rar!\x1a\x07", 6)) {
        out->format = UZ_FORMAT_RAR;
    } else if (has_magic(out, "7z\xbc\xaf\x27\x1c", 6)) {
        out->format = UZ_FORMAT_7Z;
    } else if (has_magic(out, "\x1f\x8b", 2)) {
        out->format = UZ_FORMAT_GZIP;
    } else if (has_magic(out, "BZh", 3)) {
        out->format = UZ_FORMAT_BZIP2;
    } else if (has_magic(out, "\xfd" "7zXZ\x00", 6)) {
        out->format = UZ_FORMAT_XZ;
    } else if (has_magic(out, "\x28\xb5\x2f\xfd", 4)) {
        out->format = UZ_FORMAT_ZSTD;
    } else if (has_magic(out, "\x04\x22\x4d\x18", 4)) {
        out->format = UZ_FORMAT_LZ4;
    } else if (has_magic(out, "\x1f\x9d", 2)) {
        out->format = UZ_FORMAT_COMPRESS;
    } else if (has_magic(out, "LZIP", 4)) {
        out->format = UZ_FORMAT_LZIP;
    } else if (looks_like_lzma(out)) {
        out->format = UZ_FORMAT_LZMA;
    } else if (out->magic_len >= 262 &&
               memcmp(out->magic + 257, "ustar", 5) == 0) {
        out->format = UZ_FORMAT_TAR;
    } else {
        out->format = UZ_FORMAT_UNKNOWN;
    }

    if (out->format != UZ_FORMAT_UNKNOWN && looks_like_tar(path)) {
        switch (out->format) {
        case UZ_FORMAT_GZIP:
        case UZ_FORMAT_BZIP2:
        case UZ_FORMAT_XZ:
        case UZ_FORMAT_ZSTD:
        case UZ_FORMAT_LZ4:
        case UZ_FORMAT_COMPRESS:
        case UZ_FORMAT_LZIP:
        case UZ_FORMAT_LZMA:
            out->is_tar_wrapped = 1;
            break;
        default:
            break;
        }
    }

    return out->format;
}

const char *uz_format_name(uz_format_t fmt)
{
    switch (fmt) {
    case UZ_FORMAT_ZIP:      return "zip";
    case UZ_FORMAT_RAR:      return "rar";
    case UZ_FORMAT_7Z:       return "7z";
    case UZ_FORMAT_TAR:      return "tar";
    case UZ_FORMAT_GZIP:     return "gzip";
    case UZ_FORMAT_BZIP2:    return "bzip2";
    case UZ_FORMAT_XZ:       return "xz";
    case UZ_FORMAT_ZSTD:     return "zstd";
    case UZ_FORMAT_LZ4:      return "lz4";
    case UZ_FORMAT_COMPRESS: return "compress";
    case UZ_FORMAT_LZIP:     return "lzip";
    case UZ_FORMAT_LZMA:     return "lzma";
    default:                 return "unknown";
    }
}

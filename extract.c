/* SPDX-License-Identifier: MIT */

#define _POSIX_C_SOURCE 200809L

#include "extract.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int trace_commands = 0;

static void trace_command(char *const argv[])
{
    int mask_next = 0;

    if (!trace_commands) {
        return;
    }

    fputs("uz: exec:", stderr);
    for (int i = 0; argv[i]; i++) {
        const char *p = argv[i];
        int secret = mask_next;

        mask_next = 0;
        if (!secret) {
            if (strcmp(p, "-P") == 0 || strcmp(p, "-p") == 0) {
                mask_next = 1;
            } else if (strncmp(p, "-p", 2) == 0 && p[2] != '\0' &&
                       strcmp(p, "-p-") != 0) {
                secret = 1;
            }
        }

        if (secret) {
            fputs(" '***'", stderr);
            continue;
        }

        fputs(" '", stderr);
        for (; *p; p++) {
            if (*p == '\'') {
                fputs("'\\''", stderr);
            } else {
                fputc(*p, stderr);
            }
        }
        fputc('\'', stderr);
    }
    fputc('\n', stderr);
}

static const char *seven_zip(void)
{
    static const char *const candidates[] = { "7z", "7za", "7zz", NULL };
    return uz_pick_command(candidates);
}

static int run(char *const argv[], int quiet)
{
    trace_command(argv);
    if (!uz_command_exists(argv[0])) {
        fprintf(stderr, "uz: required command not found: %s\n", argv[0]);
        return 127;
    }
    return quiet ? uz_run_quiet(argv) : uz_run(argv);
}

/* Discard stdout, keep stderr unless quiet. */
static int run_discard_stdout(char *const argv[], int quiet)
{
    trace_command(argv);
    if (!uz_command_exists(argv[0])) {
        fprintf(stderr, "uz: required command not found: %s\n", argv[0]);
        return 127;
    }
    return uz_run_discard_stdout(argv, quiet);
}

static int handle_zip(const char *path, const char *dest, int force,
                      int quiet, const char *password)
{
    char *argv[10];
    int i = 0;

    argv[i++] = "unzip";
    argv[i++] = force ? "-o" : "-n";
    if (password && *password) {
        argv[i++] = "-P";
        argv[i++] = (char *)password;
    }
    argv[i++] = (char *)path;
    argv[i++] = "-d";
    argv[i++] = (char *)dest;

    argv[i] = NULL;
    return run(argv, quiet);
}

static int handle_7z(const char *path, const char *dest, int force,
                     int quiet, const char *password)
{
    char *argv[10];
    char *pw_opt = NULL;
    char *out_opt = NULL;
    int i = 0;
    int rc;
    const char *tool = seven_zip();

    if (!tool) {
        fprintf(stderr, "uz: required command not found: 7z\n");
        return 127;
    }

    argv[i++] = (char *)tool;
    argv[i++] = "x";
    argv[i++] = "-y";
    argv[i++] = force ? "-aoa" : "-aos";
    argv[i++] = (char *)path;

    if (password && *password) {
        size_t len = strlen(password) + 3;
        pw_opt = malloc(len);
        if (!pw_opt) {
            fprintf(stderr, "uz: out of memory\n");
            return 126;
        }
        snprintf(pw_opt, len, "-p%s", password);
        argv[i++] = pw_opt;
    }

    size_t out_len = strlen(dest) + 3;
    out_opt = malloc(out_len);
    if (!out_opt) {
        fprintf(stderr, "uz: out of memory\n");
        free(pw_opt);
        return 126;
    }
    snprintf(out_opt, out_len, "-o%s", dest);
    argv[i++] = out_opt;
    argv[i] = NULL;

    rc = run(argv, quiet);
    free(pw_opt);
    free(out_opt);
    return rc;
}

static int handle_rar(const char *path, const char *dest, int force,
                      int quiet, const char *password)
{
    char *argv[10];
    char pw_opt[256];
    int i = 0;

    if (uz_command_exists("unrar")) {
        argv[i++] = "unrar";
        argv[i++] = "x";
        argv[i++] = force ? "-o+" : "-o-";
        argv[i++] = (char *)path;
        argv[i++] = (char *)dest;

        if (password && *password) {
            snprintf(pw_opt, sizeof(pw_opt), "-p%s", password);
            argv[i++] = pw_opt;
        } else {
            argv[i++] = "-p-";
        }

        argv[i] = NULL;
        return run(argv, quiet);
    }

    if (uz_command_exists("unar")) {
        argv[i++] = "unar";
        argv[i++] = "-o";
        argv[i++] = (char *)dest;
        if (force) {
            argv[i++] = "-f";
        }
        if (password && *password) {
            argv[i++] = "-p";
            argv[i++] = (char *)password;
        }
        argv[i++] = (char *)path;
        argv[i] = NULL;
        return run(argv, quiet);
    }

    if (seven_zip()) {
        return handle_7z(path, dest, force, quiet, password);
    }

    fprintf(stderr, "uz: rar extraction needs unrar, unar or 7z\n");
    return 127;
}

static const char *tar_filter(uz_format_t fmt)
{
    switch (fmt) {
    case UZ_FORMAT_GZIP:
    case UZ_FORMAT_COMPRESS:
        return "gzip";
    case UZ_FORMAT_BZIP2:
        return "bzip2";
    case UZ_FORMAT_XZ:
    case UZ_FORMAT_LZMA:
        return "xz";
    case UZ_FORMAT_ZSTD:
        return "zstd";
    case UZ_FORMAT_LZ4:
        return "lz4";
    case UZ_FORMAT_LZIP:
        return "lzip";
    default:
        return NULL;
    }
}

static int handle_tar(const char *path, const char *dest, int quiet,
                      uz_format_t fmt)
{
    char *argv[10];
    int i = 0;
    const char *filter = tar_filter(fmt);

    argv[i++] = "tar";
    argv[i++] = "-xf";
    argv[i++] = (char *)path;
    argv[i++] = "-C";
    argv[i++] = (char *)dest;

    if (filter) {
        argv[i++] = "-I";
        argv[i++] = (char *)filter;
    }

    argv[i] = NULL;
    return run(argv, quiet);
}

/* Build the output path for a single-file compressed stream. */
static char *stream_out_path(const char *path, const char *dest)
{
    char *base = strdup(path);
    char *name;
    char *dot;
    char *out;

    if (!base) {
        return NULL;
    }

    name = (char *)uz_basename(base);
    dot = strrchr(name, '.');
    if (dot && dot != name) {
        *dot = '\0';
    }

    out = (strcmp(dest, ".") == 0) ? strdup(name) : uz_path_join(dest, name);
    free(base);
    if (!out) {
        return NULL;
    }

    if (strcmp(out, path) == 0) {
        size_t need = strlen(path) + 5;
        char *fixed = malloc(need);
        if (!fixed) {
            free(out);
            return NULL;
        }
        snprintf(fixed, need, "%s.out", path);
        free(out);
        out = fixed;
    }

    return out;
}

static int run_stream(char *const argv[], const char *path, const char *dest,
                      int force, int quiet)
{
    char *outfile;
    int rc;

    trace_command(argv);
    if (!uz_command_exists(argv[0])) {
        fprintf(stderr, "uz: required command not found: %s\n", argv[0]);
        return 127;
    }

    outfile = stream_out_path(path, dest);
    if (!outfile) {
        fprintf(stderr, "uz: out of memory\n");
        return 126;
    }

    if (!force && uz_file_exists(outfile)) {
        fprintf(stderr, "uz: %s already exists (use -f to overwrite)\n",
                outfile);
        free(outfile);
        return 1;
    }

    rc = uz_run_to_file(argv, outfile, quiet);
    free(outfile);
    return rc;
}

static int handle_gzip(const char *path, const char *dest, int quiet,
                       const char *password, int force)
{
    (void)password;
    char *argv[4];
    int i = 0;

    argv[i++] = "gzip";
    argv[i++] = "-dc";
    argv[i++] = (char *)path;
    argv[i] = NULL;
    return run_stream(argv, path, dest, force, quiet);
}

static int handle_bzip2(const char *path, const char *dest, int quiet,
                        const char *password, int force)
{
    (void)password;
    char *argv[4];
    int i = 0;

    argv[i++] = "bzip2";
    argv[i++] = "-dc";
    argv[i++] = (char *)path;
    argv[i] = NULL;
    return run_stream(argv, path, dest, force, quiet);
}

static int handle_xz(const char *path, const char *dest, int quiet,
                     const char *password, int force)
{
    (void)password;
    char *argv[4];
    int i = 0;

    argv[i++] = "xz";
    argv[i++] = "-dc";
    argv[i++] = (char *)path;
    argv[i] = NULL;
    return run_stream(argv, path, dest, force, quiet);
}

static int handle_zstd(const char *path, const char *dest, int quiet,
                       const char *password, int force)
{
    (void)password;
    char *argv[4];
    int i = 0;

    argv[i++] = "zstd";
    argv[i++] = "-dc";
    argv[i++] = (char *)path;
    argv[i] = NULL;
    return run_stream(argv, path, dest, force, quiet);
}

static int handle_lz4(const char *path, const char *dest, int quiet,
                      const char *password, int force)
{
    (void)password;
    char *argv[4];
    int i = 0;

    argv[i++] = "lz4";
    argv[i++] = "-dc";
    argv[i++] = (char *)path;
    argv[i] = NULL;
    return run_stream(argv, path, dest, force, quiet);
}

static int handle_compress(const char *path, const char *dest, int quiet,
                           const char *password, int force)
{
    (void)password;
    char *argv[4];
    int i = 0;

    argv[i++] = "gzip";
    argv[i++] = "-dc";
    argv[i++] = (char *)path;
    argv[i] = NULL;
    return run_stream(argv, path, dest, force, quiet);
}

static int handle_lzip(const char *path, const char *dest, int quiet,
                       const char *password, int force)
{
    (void)password;
    char *argv[4];
    int i = 0;

    argv[i++] = "lzip";
    argv[i++] = "-dc";
    argv[i++] = (char *)path;
    argv[i] = NULL;
    return run_stream(argv, path, dest, force, quiet);
}

static int handle_lzma(const char *path, const char *dest, int quiet,
                       const char *password, int force)
{
    (void)password;
    char *argv[4];
    int i = 0;

    argv[i++] = "xz";
    argv[i++] = "-dc";
    argv[i++] = (char *)path;
    argv[i] = NULL;
    return run_stream(argv, path, dest, force, quiet);
}

static int list_archive(const uz_archive_t *arc, const char *path,
                        const char *dest, const char *password)
{
    char *argv[10];
    char pw_opt[256];
    int i = 0;
    const char *tool;

    switch (arc->format) {
    case UZ_FORMAT_ZIP:
        if (is_split_archive(path)) {
            break;
        }
        argv[i++] = "unzip";
        argv[i++] = "-l";
        if (password && *password) {
            argv[i++] = "-P";
            argv[i++] = (char *)password;
        }
        argv[i++] = (char *)path;
        argv[i] = NULL;
        return run(argv, 0);

    case UZ_FORMAT_RAR:
        if (is_split_archive(path)) {
            break;
        }
        if (uz_command_exists("unrar")) {
            argv[i++] = "unrar";
            argv[i++] = "l";
            if (password && *password) {
                snprintf(pw_opt, sizeof(pw_opt), "-p%s", password);
                argv[i++] = pw_opt;
            } else {
                argv[i++] = "-p-";
            }
            argv[i++] = (char *)path;
            argv[i] = NULL;
            return run(argv, 0);
        }
        break;

    case UZ_FORMAT_7Z:
        break;

    case UZ_FORMAT_TAR:
        argv[i++] = "tar";
        argv[i++] = "-tf";
        argv[i++] = (char *)path;
        argv[i] = NULL;
        return run(argv, 0);

    default:
        if (arc->is_tar_wrapped) {
            const char *filter = tar_filter(arc->format);
            argv[i++] = "tar";
            argv[i++] = "-tf";
            argv[i++] = (char *)path;
            if (filter) {
                argv[i++] = "-I";
                argv[i++] = (char *)filter;
            }
            argv[i] = NULL;
            return run(argv, 0);
        }
        if (arc->format == UZ_FORMAT_UNKNOWN) {
            break;
        }
        {
            char *outfile = stream_out_path(path, dest);
            if (!outfile) {
                fprintf(stderr, "uz: out of memory\n");
                return 126;
            }
            printf("%s\n", outfile);
            free(outfile);
            return 0;
        }
    }

    tool = seven_zip();
    if (!tool) {
        fprintf(stderr, "uz: required command not found: 7z\n");
        return 127;
    }
    argv[i++] = (char *)tool;
    argv[i++] = "l";
    if (password && *password) {
        snprintf(pw_opt, sizeof(pw_opt), "-p%s", password);
        argv[i++] = pw_opt;
    }
    argv[i++] = (char *)path;
    argv[i] = NULL;
    return run(argv, 0);
}

static int test_stream(const char *path, uz_format_t fmt, int quiet)
{
    char *argv[4];
    int i = 0;
    const char *tool;

    switch (fmt) {
    case UZ_FORMAT_GZIP:
    case UZ_FORMAT_COMPRESS:
        tool = "gzip";
        break;
    case UZ_FORMAT_BZIP2:
        tool = "bzip2";
        break;
    case UZ_FORMAT_XZ:
    case UZ_FORMAT_LZMA:
        tool = "xz";
        break;
    case UZ_FORMAT_ZSTD:
        tool = "zstd";
        break;
    case UZ_FORMAT_LZ4:
        tool = "lz4";
        break;
    case UZ_FORMAT_LZIP:
        tool = "lzip";
        break;
    default:
        return -1;
    }

    argv[i++] = (char *)tool;
    argv[i++] = "-t";
    argv[i++] = (char *)path;
    argv[i] = NULL;
    return run(argv, quiet);
}

static int test_archive(const uz_archive_t *arc, const char *path,
                        int quiet, const char *password)
{
    char *argv[10];
    char pw_opt[256];
    int i = 0;
    const char *tool;

    switch (arc->format) {
    case UZ_FORMAT_ZIP:
        if (is_split_archive(path)) {
            break;
        }
        argv[i++] = "unzip";
        argv[i++] = "-t";
        if (password && *password) {
            argv[i++] = "-P";
            argv[i++] = (char *)password;
        }
        argv[i++] = (char *)path;
        argv[i] = NULL;
        return run(argv, quiet);

    case UZ_FORMAT_RAR:
        if (is_split_archive(path)) {
            break;
        }
        if (uz_command_exists("unrar")) {
            argv[i++] = "unrar";
            argv[i++] = "t";
            argv[i++] = (char *)path;
            if (password && *password) {
                snprintf(pw_opt, sizeof(pw_opt), "-p%s", password);
                argv[i++] = pw_opt;
            } else {
                argv[i++] = "-p-";
            }
            argv[i] = NULL;
            return run(argv, quiet);
        }
        break;

    case UZ_FORMAT_7Z:
        break;

    case UZ_FORMAT_TAR:
        argv[i++] = "tar";
        argv[i++] = "-tf";
        argv[i++] = (char *)path;
        argv[i] = NULL;
        return run_discard_stdout(argv, quiet);

    default:
        if (arc->is_tar_wrapped) {
            const char *filter = tar_filter(arc->format);
            argv[i++] = "tar";
            argv[i++] = "-tf";
            argv[i++] = (char *)path;
            if (filter) {
                argv[i++] = "-I";
                argv[i++] = (char *)filter;
            }
            argv[i] = NULL;
            return run_discard_stdout(argv, quiet);
        }
        if (arc->format == UZ_FORMAT_UNKNOWN) {
            break;
        }
        return test_stream(path, arc->format, quiet);
    }

    tool = seven_zip();
    if (!tool) {
        fprintf(stderr, "uz: required command not found: 7z\n");
        return 127;
    }
    argv[i++] = (char *)tool;
    argv[i++] = "t";
    if (password && *password) {
        snprintf(pw_opt, sizeof(pw_opt), "-p%s", password);
        argv[i++] = pw_opt;
    }
    argv[i++] = (char *)path;
    argv[i] = NULL;
    return run(argv, quiet);
}

static char *path_dirname(const char *path)
{
    const char *slash = strrchr(path, '/');

    if (!slash) {
        return strdup(".");
    }
    if (slash == path) {
        return strdup("/");
    }

    size_t len = (size_t)(slash - path);
    char *dir = malloc(len + 1);
    if (!dir) {
        return NULL;
    }
    memcpy(dir, path, len);
    dir[len] = '\0';
    return dir;
}

static int strip_suffix(char *name, const char *suffix)
{
    size_t name_len = strlen(name);
    size_t suffix_len = strlen(suffix);

    if (name_len <= suffix_len ||
        strcmp(name + name_len - suffix_len, suffix) != 0) {
        return 0;
    }

    name[name_len - suffix_len] = '\0';
    return 1;
}

static void strip_volume_suffix(char *name)
{
    size_t len = strlen(name);

    if (len > 4 &&
        name[len - 4] == '.' &&
        name[len - 3] >= '0' && name[len - 3] <= '9' &&
        name[len - 2] >= '0' && name[len - 2] <= '9' &&
        name[len - 1] >= '0' && name[len - 1] <= '9') {
        name[len - 4] = '\0';
    }
}

static void strip_archive_suffix(char *name)
{
    static const char *const suffixes[] = {
        ".tar.gz", ".tar.bz2", ".tar.xz", ".tar.zst", ".tar.zstd",
        ".tar.lz4", ".tar.lz", ".tar.lzma", ".tar.Z",
        ".tgz", ".tbz", ".tbz2", ".txz", ".tzst", ".tlz4", ".tlz",
        ".tar",
        ".zip", ".rar", ".7z",
        ".gz", ".bz2", ".xz", ".zst", ".zstd", ".lz4", ".lz",
        ".lzma", ".Z",
        NULL
    };

    for (size_t i = 0; suffixes[i]; i++) {
        if (strip_suffix(name, suffixes[i])) {
            return;
        }
    }
}

static int is_bare_stream(uz_format_t fmt, int is_tar_wrapped)
{
    if (is_tar_wrapped) {
        return 0;
    }

    switch (fmt) {
    case UZ_FORMAT_GZIP:
    case UZ_FORMAT_BZIP2:
    case UZ_FORMAT_XZ:
    case UZ_FORMAT_ZSTD:
    case UZ_FORMAT_LZ4:
    case UZ_FORMAT_COMPRESS:
    case UZ_FORMAT_LZIP:
    case UZ_FORMAT_LZMA:
        return 1;
    default:
        return 0;
    }
}

char *uz_default_dest(const char *path, uz_format_t fmt, int is_tar_wrapped)
{
    char *dir = path_dirname(path);
    if (!dir) {
        return NULL;
    }

    /* A bare stream is written as a single file next to the archive. */
    if (is_bare_stream(fmt, is_tar_wrapped)) {
        return dir;
    }

    char *name = strdup(uz_basename(path));
    if (!name) {
        free(dir);
        return NULL;
    }

    strip_volume_suffix(name);
    strip_archive_suffix(name);
    if (!*name) {
        free(name);
        name = strdup("archive");
        if (!name) {
            free(dir);
            return NULL;
        }
    }

    char *dest;
    if (strcmp(dir, ".") == 0) {
        dest = strdup(name);
    } else {
        dest = uz_path_join(dir, name);
    }

    free(dir);
    free(name);
    if (!dest) {
        return NULL;
    }

    /* Avoid colliding with an archive that has no recognizable suffix. */
    if (strcmp(dest, path) == 0) {
        size_t need = strlen(dest) + 11;
        char *alt = malloc(need);
        if (!alt) {
            free(dest);
            return NULL;
        }
        snprintf(alt, need, "%s.extracted", dest);
        free(dest);
        dest = alt;
    }

    return dest;
}

int uz_process(const uz_archive_t *arc, const char *path, const char *dest,
               int force, int quiet, int verbose, uz_mode_t mode,
               const char *password)
{
    if (!dest || !*dest) return -1;

    trace_commands = verbose;

    if (mode == UZ_MODE_LIST) {
        return list_archive(arc, path, dest, password);
    }
    if (mode == UZ_MODE_TEST) {
        return test_archive(arc, path, quiet, password);
    }

    uz_mkdir_p(dest);

    if (is_split_archive(path)) {
        if (seven_zip()) {
            return handle_7z(path, dest, force, quiet, password);
        }
        if (arc->format == UZ_FORMAT_RAR) {
            return handle_rar(path, dest, force, quiet, password);
        }
        fprintf(stderr, "uz: split archives require 7z\n");
        return 127;
    }

    if (arc->is_tar_wrapped) {
        return handle_tar(path, dest, quiet, arc->format);
    }

    switch (arc->format) {
        case UZ_FORMAT_ZIP:
            return handle_zip(path, dest, force, quiet, password);
        case UZ_FORMAT_RAR:
            return handle_rar(path, dest, force, quiet, password);
        case UZ_FORMAT_7Z:
            return handle_7z(path, dest, force, quiet, password);
        case UZ_FORMAT_TAR:
            return handle_tar(path, dest, quiet, UZ_FORMAT_TAR);
        case UZ_FORMAT_GZIP:
            return handle_gzip(path, dest, quiet, password, force);
        case UZ_FORMAT_BZIP2:
            return handle_bzip2(path, dest, quiet, password, force);
        case UZ_FORMAT_XZ:
            return handle_xz(path, dest, quiet, password, force);
        case UZ_FORMAT_ZSTD:
            return handle_zstd(path, dest, quiet, password, force);
        case UZ_FORMAT_LZ4:
            return handle_lz4(path, dest, quiet, password, force);
        case UZ_FORMAT_COMPRESS:
            return handle_compress(path, dest, quiet, password, force);
        case UZ_FORMAT_LZIP:
            return handle_lzip(path, dest, quiet, password, force);
        case UZ_FORMAT_LZMA:
            return handle_lzma(path, dest, quiet, password, force);
        default:
            return handle_7z(path, dest, force, quiet, password);
    }
}

int is_split_archive(const char *path)
{
    const char *base = uz_basename(path);

    size_t len = strlen(base);
    if (len < 5) return 0;

    const char *suffix = base + len - 4;
    if (suffix[0] != '.' ||
        suffix[1] < '0' || suffix[1] > '9' ||
        suffix[2] < '0' || suffix[2] > '9' ||
        suffix[3] < '0' || suffix[3] > '9') {
        return 0;
    }

    return 1;
}

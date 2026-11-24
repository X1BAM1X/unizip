/* SPDX-License-Identifier: MIT */

#include "detect.h"
#include "extract.h"
#include "unizip.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void usage(FILE *out, const char *prog)
{
    fprintf(out,
            "Usage: %s [options] <archive> [destination]\n"
            "\n"
            "Extract or inspect an archive using the system's own tools.\n"
            "\n"
            "Options:\n"
            "  -o, --output DIR   extract into DIR (default: next to the archive)\n"
            "  -f, --force        overwrite existing files\n"
            "  -l, --list         list archive contents\n"
            "  -t, --test         test archive integrity\n"
            "  -q, --quiet        suppress tool output\n"
            "  -v, --verbose      show detected format and executed commands\n"
            "  -p, --password PW  password for encrypted archives\n"
            "  -h, --help         show this help\n"
            "  -V, --version      show version\n"
            "\n"
            "Exit status: 0 ok, 1 usage, 2 failed, 3 missing tool, 4 internal.\n"
            "\n"
            "By default container archives go into a directory named after the\n"
            "archive, next to it. Bare compressed streams stay in that directory.\n"
            "\n"
            "Supported: zip, rar, 7z, tar, gzip, bzip2, xz, zstd, lz4,\n"
            "           compress(.Z), lzip, lzma and tar wrapped in the above.\n"
            "Multi-volume archives (e.g. archive.zip.001) are automatically detected.\n"
            "Unknown formats are handed to 7z when it is available.\n",
            prog);
}

static int parse_args(int argc, char **argv, const char **archive,
                      const char **dest, int *force, int *quiet, int *verbose,
                      uz_mode_t *mode, const char **password)
{
    int options_done = 0;
    int dest_set = 0;

    *archive = NULL;
    *dest = NULL;
    *force = 0;
    *quiet = 0;
    *verbose = 0;
    *mode = UZ_MODE_EXTRACT;
    *password = NULL;

    for (int i = 1; i < argc; i++) {
        char *arg = argv[i];

        if (!options_done && arg[0] == '-' && arg[1] != '\0') {
            if (strcmp(arg, "--") == 0) {
                options_done = 1;
            } else if (strcmp(arg, "-h") == 0 ||
                       strcmp(arg, "--help") == 0) {
                usage(stdout, argv[0]);
                return 1;
            } else if (strcmp(arg, "-V") == 0 ||
                       strcmp(arg, "--version") == 0) {
                printf("uz %s\n", UZ_VERSION);
                return 1;
            } else if (strcmp(arg, "-o") == 0 ||
                       strcmp(arg, "--output") == 0) {
                if (++i >= argc) {
                    fprintf(stderr, "uz: option '%s' needs a directory\n", arg);
                    return -1;
                }
                *dest = argv[i];
                dest_set = 1;
            } else if (strncmp(arg, "--output=", 9) == 0) {
                *dest = arg + 9;
                dest_set = 1;
            } else if (strcmp(arg, "-f") == 0 ||
                       strcmp(arg, "--force") == 0) {
                *force = 1;
            } else if (strcmp(arg, "-l") == 0 ||
                       strcmp(arg, "--list") == 0) {
                *mode = UZ_MODE_LIST;
            } else if (strcmp(arg, "-t") == 0 ||
                       strcmp(arg, "--test") == 0) {
                *mode = UZ_MODE_TEST;
            } else if (strcmp(arg, "-q") == 0 ||
                       strcmp(arg, "--quiet") == 0) {
                *quiet = 1;
            } else if (strcmp(arg, "-v") == 0 ||
                       strcmp(arg, "--verbose") == 0) {
                *verbose = 1;
            } else if (strcmp(arg, "-p") == 0 ||
                       strcmp(arg, "--password") == 0) {
                if (++i >= argc) {
                    fprintf(stderr, "uz: option '%s' needs a password\n", arg);
                    return -1;
                }
                *password = argv[i];
            } else if (strncmp(arg, "--password=", 11) == 0) {
                *password = arg + 11;
            } else {
                fprintf(stderr, "uz: unknown option '%s'\n", arg);
                usage(stderr, argv[0]);
                return -1;
            }
        } else if (!*archive) {
            *archive = arg;
        } else if (!dest_set) {
            *dest = arg;
            dest_set = 1;
        } else {
            fprintf(stderr, "uz: too many arguments\n");
            usage(stderr, argv[0]);
            return -1;
        }
    }

    if (!*archive) {
        fprintf(stderr, "uz: no archive specified\n");
        usage(stderr, argv[0]);
        return -1;
    }
    return 0;
}

static int map_status(int rc)
{
    if (rc == 0) {
        return UZ_EXIT_OK;
    }
    if (rc == 127) {
        return UZ_EXIT_TOOL_MISSING;
    }
    if (rc == 126) {
        return UZ_EXIT_INTERNAL;
    }
    if (rc < 0) {
        return UZ_EXIT_USAGE;
    }
    return UZ_EXIT_FAILURE;
}

int main(int argc, char **argv)
{
    const char *archive = NULL;
    const char *dest = NULL;
    int force = 0;
    int quiet = 0;
    int verbose = 0;
    uz_mode_t mode = UZ_MODE_EXTRACT;
    const char *password = NULL;

    int parsed = parse_args(argc, argv, &archive, &dest, &force, &quiet,
                            &verbose, &mode, &password);
    if (parsed != 0) {
        return parsed > 0 ? 0 : 1;
    }

    /* Keep paths starting with '-' out of the child commands' option parser. */
    char *normalized = NULL;
    if (archive[0] == '-') {
        size_t need = strlen(archive) + 3;
        normalized = malloc(need);
        if (!normalized) {
            fprintf(stderr, "uz: out of memory\n");
            return 1;
        }
        snprintf(normalized, need, "./%s", archive);
        archive = normalized;
    }

    if (access(archive, R_OK) != 0) {
        perror(archive);
        free(normalized);
        return UZ_EXIT_FAILURE;
    }

    uz_archive_t arc;
    uz_format_t fmt = uz_detect_format(archive, &arc);

    if (fmt == UZ_FORMAT_UNKNOWN) {
        int has_7z = uz_command_exists("7z") || uz_command_exists("7za") ||
                     uz_command_exists("7zz");
        if (!has_7z) {
            fprintf(stderr, "uz: unknown or unsupported archive format: %s\n",
                    archive);
            free(normalized);
            return UZ_EXIT_TOOL_MISSING;
        }
        if (verbose) {
            fprintf(stderr, "uz: %s has unknown magic, falling back to 7z\n",
                    archive);
        }
    } else if (verbose) {
        fprintf(stderr, "uz: %s detected as %s%s\n", archive,
                uz_format_name(fmt),
                arc.is_tar_wrapped ? " (tar stream)" : "");
    }

    char *auto_dest = NULL;
    if (!dest) {
        auto_dest = uz_default_dest(archive, fmt, arc.is_tar_wrapped);
        if (!auto_dest) {
            fprintf(stderr, "uz: out of memory\n");
            free(normalized);
            return UZ_EXIT_INTERNAL;
        }
        dest = auto_dest;
    }

    int rc = uz_process(&arc, archive, dest, force, quiet, verbose, mode,
                        password);
    free(auto_dest);
    free(normalized);
    return map_status(rc);
}

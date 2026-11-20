/* SPDX-License-Identifier: MIT */

#ifndef UZ_EXTRACT_H
#define UZ_EXTRACT_H

#include "unizip.h"

int uz_process(const uz_archive_t *arc, const char *path, const char *dest,
               int force, int quiet, int verbose, uz_mode_t mode,
               const char *password);

char *uz_default_dest(const char *path, uz_format_t fmt, int is_tar_wrapped);

int is_split_archive(const char *path);

#endif

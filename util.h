/* SPDX-License-Identifier: MIT */

#ifndef UZ_UTIL_H
#define UZ_UTIL_H

int uz_command_exists(const char *cmd);
const char *uz_pick_command(const char *const *candidates);

int uz_run(char *const argv[]);
int uz_run_quiet(char *const argv[]);
int uz_run_discard_stdout(char *const argv[], int quiet);
int uz_run_to_file(char *const argv[], const char *outfile, int quiet);

int uz_mkdir_p(const char *path);
int uz_file_exists(const char *path);
int uz_ends_with(const char *text, const char *suffix);

const char *uz_basename(const char *path);
char *uz_path_join(const char *dir, const char *name);

#endif

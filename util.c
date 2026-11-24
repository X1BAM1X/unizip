/* SPDX-License-Identifier: MIT */

#define _POSIX_C_SOURCE 200809L
#include <sys/wait.h>
#include <sys/stat.h>

#include "util.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef O_NOFOLLOW
#define O_NOFOLLOW 0
#endif

int uz_command_exists(const char *cmd)
{
    const char *path_env = getenv("PATH");
    if (!path_env) {
        return 0;
    }

    char *path_copy = strdup(path_env);
    if (!path_copy) {
        return 0;
    }

    char *saveptr;
    char *dir = strtok_r(path_copy, ":", &saveptr);
    while (dir) {
        size_t dir_len = strlen(dir);
        if (dir_len > 0 && dir[dir_len - 1] == '/') {
            dir_len--;
        }
        if (dir_len == 0) {
            dir = strtok_r(NULL, ":", &saveptr);
            continue;
        }

        size_t need = dir_len + 1 + strlen(cmd) + 1;
        char *path = malloc(need);
        if (!path) {
            free(path_copy);
            return 0;
        }
        snprintf(path, need, "%.*s/%s", (int)dir_len, dir, cmd);

        int found = access(path, X_OK) == 0;
        free(path);
        if (found) {
            free(path_copy);
            return 1;
        }
        dir = strtok_r(NULL, ":", &saveptr);
    }

    free(path_copy);
    return 0;
}

const char *uz_pick_command(const char *const *candidates)
{
    for (int i = 0; candidates[i]; i++) {
        if (uz_command_exists(candidates[i])) {
            return candidates[i];
        }
    }
    return NULL;
}

static int wait_child(int pid)
{
    int status;

    if (waitpid(pid, &status, 0) < 0) {
        return 125;
    }
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    if (WIFSIGNALED(status)) {
        return 128 + WTERMSIG(status);
    }
    return 125;
}

/* uz never supports interactive child commands; do not let them wait on a tty. */
static void redirect_stdin_devnull(void)
{
    int devnull = open("/dev/null", O_RDONLY);
    if (devnull >= 0) {
        dup2(devnull, STDIN_FILENO);
        close(devnull);
    }
}

int uz_run(char *const argv[])
{
    int pid = fork();
    if (pid == 0) {
        redirect_stdin_devnull();
        execvp(argv[0], argv);
        _exit(127);
    }
    if (pid < 0) {
        return 126;
    }
    return wait_child(pid);
}

int uz_run_quiet(char *const argv[])
{
    int pid = fork();
    if (pid == 0) {
        redirect_stdin_devnull();
        int devnull = open("/dev/null", O_WRONLY);
        if (devnull < 0) {
            _exit(127);
        }
        dup2(devnull, STDOUT_FILENO);
        dup2(devnull, STDERR_FILENO);
        close(devnull);
        execvp(argv[0], argv);
        _exit(127);
    }
    if (pid < 0) {
        return 126;
    }
    return wait_child(pid);
}

int uz_run_discard_stdout(char *const argv[], int quiet)
{
    int pid = fork();
    if (pid == 0) {
        redirect_stdin_devnull();
        int devnull = open("/dev/null", O_WRONLY);
        if (devnull < 0) {
            _exit(127);
        }
        dup2(devnull, STDOUT_FILENO);
        if (quiet) {
            dup2(devnull, STDERR_FILENO);
        }
        close(devnull);
        execvp(argv[0], argv);
        _exit(127);
    }
    if (pid < 0) {
        return 126;
    }
    return wait_child(pid);
}

static int ensure_dir(const char *path)
{
    if (mkdir(path, 0755) == 0) {
        return 0;
    }
    if (errno == EEXIST) {
        struct stat st;
        if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
            return 0;
        }
        errno = ENOTDIR;
    }
    return -1;
}

int uz_mkdir_p(const char *path)
{
    char *tmp;
    char *p = NULL;
    size_t len;
    int rc = 0;

    if (!path || !*path) {
        return -1;
    }

    tmp = strdup(path);
    if (!tmp) {
        return -1;
    }

    len = strlen(tmp);
    if (len > 1 && tmp[len - 1] == '/') {
        tmp[len - 1] = '\0';
    }

    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (ensure_dir(tmp) != 0) {
                rc = -1;
                goto done;
            }
            *p = '/';
        }
    }

    if (ensure_dir(tmp) != 0) {
        rc = -1;
    }

done:
    free(tmp);
    return rc;
}

int uz_file_exists(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0;
}

int uz_ends_with(const char *text, const char *suffix)
{
    size_t text_len = strlen(text);
    size_t suffix_len = strlen(suffix);

    if (suffix_len > text_len) {
        return 0;
    }

    return strcmp(text + text_len - suffix_len, suffix) == 0;
}

const char *uz_basename(const char *path)
{
    if (!path) {
        return "";
    }
    const char *slash = strrchr(path, '/');
    return slash ? slash + 1 : path;
}

char *uz_path_join(const char *dir, const char *name)
{
    if (!dir || !name) {
        return NULL;
    }

    size_t dir_len = strlen(dir);
    size_t name_len = strlen(name);
    size_t sep = (dir_len > 0 && dir[dir_len - 1] == '/') ? 0 : 1;
    char *result = malloc(dir_len + sep + name_len + 1);
    if (!result) {
        return NULL;
    }

    memcpy(result, dir, dir_len);
    if (sep) {
        result[dir_len] = '/';
    }
    memcpy(result + dir_len + sep, name, name_len + 1);
    return result;
}

int uz_run_to_file(char *const argv[], const char *outfile, int quiet)
{
    struct stat st;

    if (lstat(outfile, &st) == 0 && S_ISLNK(st.st_mode)) {
        fprintf(stderr, "uz: refusing to overwrite symlink: %s\n", outfile);
        return ELOOP;
    }

    size_t tmplen = strlen(outfile) + 64;
    char *tmpfile = malloc(tmplen);
    if (!tmpfile) {
        fprintf(stderr, "uz: out of memory\n");
        return 126;
    }

    int fd = -1;
    for (unsigned i = 0; i < 100; i++) {
        snprintf(tmpfile, tmplen, "%s.tmp.%ld.%u",
                 outfile, (long)getpid(), i);
        fd = open(tmpfile, O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW, 0644);
        if (fd >= 0) {
            break;
        }
        if (errno != EEXIST) {
            break;
        }
    }

    if (fd < 0) {
        int err = errno;
        errno = err;
        perror(outfile);
        free(tmpfile);
        return err;
    }

    int pipe_to_file[2];
    if (pipe(pipe_to_file) == -1) {
        int err = errno;
        perror("pipe");
        close(fd);
        unlink(tmpfile);
        free(tmpfile);
        return err;
    }

    int pid = fork();
    if (pid == 0) {
        redirect_stdin_devnull();
        close(pipe_to_file[0]);
        dup2(pipe_to_file[1], STDOUT_FILENO);
        close(pipe_to_file[1]);
        if (quiet) {
            int devnull = open("/dev/null", O_WRONLY);
            if (devnull >= 0) {
                dup2(devnull, STDERR_FILENO);
                close(devnull);
            }
        }
        execvp(argv[0], argv);
        _exit(127);
    }

    if (pid < 0) {
        close(pipe_to_file[0]);
        close(pipe_to_file[1]);
        close(fd);
        unlink(tmpfile);
        free(tmpfile);
        return 126;
    }

    close(pipe_to_file[1]);

    ssize_t n;
    char buf[8192];
    int io_failed = 0;

    for (;;) {
        n = read(pipe_to_file[0], buf, sizeof(buf));
        if (n == 0) {
            break;
        }
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            io_failed = 1;
            break;
        }

        ssize_t off = 0;
        while (off < n) {
            ssize_t written = write(fd, buf + off, (size_t)(n - off));
            if (written <= 0) {
                if (written < 0 && errno == EINTR) {
                    continue;
                }
                io_failed = 1;
                break;
            }
            off += written;
        }
        if (io_failed) {
            break;
        }
    }

    close(pipe_to_file[0]);
    close(fd);

    int status;
    waitpid(pid, &status, 0);

    int result;
    if (io_failed) {
        result = 126;
    } else if (WIFEXITED(status)) {
        result = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        result = 128 + WTERMSIG(status);
    } else {
        result = 125;
    }

    if (result == 0) {
        if (rename(tmpfile, outfile) != 0) {
            int err = errno;
            errno = err;
            perror(outfile);
            unlink(tmpfile);
            result = err;
        }
    } else {
        unlink(tmpfile);
    }

    free(tmpfile);
    return result;
}

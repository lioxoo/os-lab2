#include "proclaunch.h"

#ifdef _WIN32
/* Windows-реализация */
#include <windows.h>

int pl_start(const char *const argv[], pl_process *proc) {
    /* TODO: реализовать CreateProcess */
    return -1;
}

int pl_wait(pl_process *proc, int *exit_code) {
    /* TODO: реализовать WaitForSingleObject + GetExitCodeProcess */
    return -1;
}

int pl_poll(pl_process *proc, int *exit_code) {
    /* TODO: реализовать WaitForSingleObject с таймаутом 0 */
    return -1;
}

#else
/* POSIX-реализация */
#include <unistd.h>
#include <sys/wait.h>
#include <spawn.h>
#include <errno.h>

extern char **environ;

int pl_start(const char *const argv[], pl_process *proc) {
    if (!argv || !argv[0] || !proc) return -1;

    pid_t pid;
    int err = posix_spawnp(&pid, argv[0], NULL, NULL,
                           (char *const *)argv, environ);
    if (err != 0) return -1;   /* posix_spawnp сам вернёт ENOENT для несуществующей программы */

    proc->pid = (long)pid;
    proc->handle = 0;
    return 0;
}

int pl_wait(pl_process *proc, int *exit_code) {
    if (!proc || !exit_code) return -1;

    int status;
    pid_t r;
    do {
        r = waitpid((pid_t)proc->pid, &status, 0);
    } while (r < 0 && errno == EINTR);

    if (r < 0) return -1;

    if (WIFEXITED(status))
        *exit_code = WEXITSTATUS(status);
    else if (WIFSIGNALED(status))
        *exit_code = 128 + WTERMSIG(status);
    else
        *exit_code = -1;

    return 0;
}

int pl_poll(pl_process *proc, int *exit_code) {
    if (!proc || !exit_code) return -1;

    int status;
    pid_t r = waitpid((pid_t)proc->pid, &status, WNOHANG);

    if (r < 0) return -1;
    if (r == 0) return 0;   /* ещё работает */

    if (WIFEXITED(status))
        *exit_code = WEXITSTATUS(status);
    else if (WIFSIGNALED(status))
        *exit_code = 128 + WTERMSIG(status);
    else
        *exit_code = -1;

    return 1;   /* завершился */
}

#endif
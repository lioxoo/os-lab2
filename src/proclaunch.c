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

int pl_start(const char *const argv[], pl_process *proc) {
    /* TODO: реализовать fork + execvp */
    return -1;
}

int pl_wait(pl_process *proc, int *exit_code) {
    /* TODO: реализовать waitpid */
    return -1;
}

int pl_poll(pl_process *proc, int *exit_code) {
    /* TODO: реализовать waitpid с WNOHANG */
    return -1;
}

#endif
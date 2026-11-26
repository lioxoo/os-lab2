#include "proclaunch.h"

#ifdef _WIN32
/* Windows-реализация */
#include <windows.h>
#include <stdlib.h>
#include <string.h>

/* Построить командную строку из argv с учётом кавычек */
static char *build_cmdline(const char *const argv[]) {
    size_t total = 1;
    for (int i = 0; argv[i]; i++) {
        total += strlen(argv[i]) * 2 + 3;
    }
    char *cmd = (char *)malloc(total);
    if (!cmd) return NULL;

    char *p = cmd;
    for (int i = 0; argv[i]; i++) {
        if (i > 0) *p++ = ' ';
        const char *s = argv[i];
        int need_quotes = (*s == '\0') || strpbrk(s, " \t\"") != NULL;
        if (need_quotes) {
            *p++ = '"';
            for (; *s; s++) {
                int backslashes = 0;
                while (*s == '\\') { backslashes++; *p++ = *s++; }
                if (*s == '"') {
                    for (int k = 0; k <= backslashes; k++) *p++ = '\\';
                    *p++ = '"';
                } else {
                    for (int k = 0; k < backslashes; k++) *p++ = '\\';
                    if (*s) *p++ = *s;
                }
            }
            *p++ = '"';
        } else {
            strcpy(p, s);
            p += strlen(s);
        }
    }
    *p = '\0';
    return cmd;
}

int pl_start(const char *const argv[], pl_process *proc) {
    if (!argv || !argv[0] || !proc) return -1;

    char *cmd = build_cmdline(argv);
    if (!cmd) return -1;

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof si);
    si.cb = sizeof si;

    BOOL ok = CreateProcessA(NULL, cmd, NULL, NULL, FALSE, 0,
                             NULL, NULL, &si, &pi);
    free(cmd);

    if (!ok) return -1;   /* GetLastError() == 2 — файл не найден */

    CloseHandle(pi.hThread);
    proc->handle = (intptr_t)pi.hProcess;
    proc->pid = (long)pi.dwProcessId;
    return 0;
}

int pl_wait(pl_process *proc, int *exit_code) {
    if (!proc || !exit_code) return -1;

    HANDLE h = (HANDLE)proc->handle;
    DWORD r = WaitForSingleObject(h, INFINITE);
    if (r != WAIT_OBJECT_0) return -1;

    DWORD code = 0;
    if (!GetExitCodeProcess(h, &code)) return -1;
    *exit_code = (int)code;

    CloseHandle(h);
    proc->handle = 0;
    return 0;
}

int pl_poll(pl_process *proc, int *exit_code) {
    if (!proc || !exit_code) return -1;

    HANDLE h = (HANDLE)proc->handle;
    DWORD r = WaitForSingleObject(h, 0);

    if (r == WAIT_TIMEOUT) return 0;       /* ещё работает */
    if (r != WAIT_OBJECT_0) return -1;

    DWORD code = 0;
    if (!GetExitCodeProcess(h, &code)) return -1;
    *exit_code = (int)code;

    CloseHandle(h);
    proc->handle = 0;
    return 1;   /* завершился */
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
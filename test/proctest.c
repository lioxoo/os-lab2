#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "proclaunch.h"

#ifdef _WIN32
#include <windows.h>
static void sleep_ms(int ms) { Sleep(ms); }
#else
#include <time.h>
static void sleep_ms(int ms) {
    struct timespec ts = { ms / 1000, (ms % 1000) * 1000000L };
    nanosleep(&ts, NULL);
}
#endif

static int failures = 0;

static void check(int cond, const char *name) {
    printf("[%s] %s\n", cond ? "OK  " : "FAIL", name);
    if (!cond) failures++;
}

int main(int argc, char **argv) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    /* Режим ребёнка: --child <код> <мс> [аргументы...] */
    if (argc >= 4 && strcmp(argv[1], "--child") == 0) {
        int code = atoi(argv[2]);
        int ms = atoi(argv[3]);
        sleep_ms(ms);
        return code;
    }

    /* Режим проверки аргументов: --check-args <arg1> <arg2> <arg3> */
    if (argc >= 5 && strcmp(argv[1], "--check-args") == 0) {
        int ok = (strcmp(argv[2], "hello") == 0 &&
                  strcmp(argv[3], "big world") == 0 &&
                  strcmp(argv[4], "test") == 0);
        return ok ? 0 : 1;
    }

    const char *self = argv[0];

    /* Тест 1: фоновый запуск, pl_poll в цикле */
    {
        char code[8], ms[8];
        snprintf(code, sizeof code, "%d", 42);
        snprintf(ms, sizeof ms, "%d", 1000);
        const char *child_argv[] = { self, "--child", code, ms, NULL };

        pl_process proc;
        int rc = pl_start(child_argv, &proc);
        check(rc == 0, "Test1: pl_start вернул 0");

        int iterations = 0;
        int exit_code = -1;
        int done = 0;
        while (!done && iterations < 100) {
            sleep_ms(100);
            iterations++;
            int r = pl_poll(&proc, &exit_code);
            if (r == 1) done = 1;
            else if (r == -1) break;
        }
        check(iterations > 3, "Test1: родитель работал, пока ребёнок спал");
        check(done && exit_code == 42, "Test1: код возврата 42");
    }

    /* Тест 2: pl_wait */
    {
        char code[8], ms[8];
        snprintf(code, sizeof code, "%d", 7);
        snprintf(ms, sizeof ms, "%d", 300);
        const char *child_argv[] = { self, "--child", code, ms, NULL };

        pl_process proc;
        int rc = pl_start(child_argv, &proc);
        check(rc == 0, "Test2: pl_start вернул 0");

        int exit_code = -1;
        rc = pl_wait(&proc, &exit_code);
        check(rc == 0 && exit_code == 7, "Test2: pl_wait вернул код 7");
    }

    /* Тест 3: аргументы с пробелами */
    {
        const char *child_argv[] = {
            self, "--check-args", "hello", "big world", "test", NULL
        };
        pl_process proc;
        int rc = pl_start(child_argv, &proc);
        check(rc == 0, "Test3: запуск с пробелами");

        int exit_code = -1;
        pl_wait(&proc, &exit_code);
        check(exit_code == 0, "Test3: ребёнок получил аргументы без искажений");
    }

    /* Тест 4: несуществующая программа */
    {
        const char *child_argv[] = { "no_such_program_12345", NULL };
        pl_process proc;
        int rc = pl_start(child_argv, &proc);
        check(rc == -1, "Test4: ошибка при запуске несуществующей программы");
    }

    /* Тест 5: код 300 */
    {
        char code[8], ms[8];
        snprintf(code, sizeof code, "%d", 300);
        snprintf(ms, sizeof ms, "%d", 0);
        const char *child_argv[] = { self, "--child", code, "0", NULL };

        pl_process proc;
        pl_start(child_argv, &proc);
        int exit_code = -1;
        pl_wait(&proc, &exit_code);
#ifdef _WIN32
        check(exit_code == 300, "Test5: Windows вернул 300");
#else
        check(exit_code == 44, "Test5: Linux обрезал 300 до 44 (8 бит)");
#endif
    }

    printf("\n%s\n", failures == 0 ? "ALL TESTS PASSED" : "SOME TESTS FAILED");
    return failures == 0 ? 0 : 1;
}
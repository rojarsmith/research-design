/*
 * pipeline.c - Stage 02 companion for Linux (WSL 2 or a Linux VM):
 * "cat pipeline.c | grep pipe | wc -l" with fork, exec, pipe and wait,
 * the same shape as xv6/pipeline.c.
 *
 * Built and run on 2026-11-16 in an Ubuntu 24.04 container (gcc 13.3.0,
 * glibc 2.39, strace 6.8); output in linux/example-output.txt.
 *
 * Build:  gcc -O2 -Wall -o pipeline pipeline.c
 * Run:    ./pipeline                 (compare: cat pipeline.c | grep pipe | wc -l)
 *         ./pipeline --leak          (the parent keeps every pipe end open; Ctrl-C to stop)
 *         strace -f -e trace=clone,clone3,execve,pipe,pipe2,dup2,wait4 ./pipeline
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static pid_t spawn(char *const argv[], int in, int out, const int *fds, int nfds)
{
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(1);
    }
    if (pid == 0) {
        if (in >= 0 && dup2(in, 0) < 0) {
            perror("dup2");
            _exit(1);
        }
        if (out >= 0 && dup2(out, 1) < 0) {
            perror("dup2");
            _exit(1);
        }
        for (int i = 0; i < nfds; i++)
            close(fds[i]);
        execvp(argv[0], argv);
        perror(argv[0]);
        _exit(127);
    }
    return pid;
}

int main(int argc, char **argv)
{
    int leak = argc > 1 && strcmp(argv[1], "--leak") == 0;
    int a[2], b[2];
    if (pipe(a) < 0 || pipe(b) < 0) {
        perror("pipe");
        return 1;
    }
    int fds[4] = {a[0], a[1], b[0], b[1]};
    char *cat[] = {"cat", "pipeline.c", NULL};
    char *grep[] = {"grep", "pipe", NULL};
    char *wc[] = {"wc", "-l", NULL};

    pid_t p1 = spawn(cat, -1, a[1], fds, 4);
    pid_t p2 = spawn(grep, a[0], b[1], fds, 4);
    pid_t p3 = spawn(wc, b[0], -1, fds, 4);
    fprintf(stderr, "spawned cat %d, grep %d, wc %d\n", (int)p1, (int)p2, (int)p3);

    if (leak)
        fprintf(stderr, "--leak: the parent keeps every pipe end open\n");
    else
        for (int i = 0; i < 4; i++)
            close(fds[i]);

    for (int i = 0; i < 3; i++) {
        int st;
        pid_t pid = wait(&st);
        fprintf(stderr, "pid %d exited with status %d\n", (int)pid,
                WIFEXITED(st) ? WEXITSTATUS(st) : -1);
    }
    return 0;
}

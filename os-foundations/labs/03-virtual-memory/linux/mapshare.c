/*
 * mapshare.c - Stage 03, exercise D: one file mapped by two processes,
 * first both MAP_SHARED, then one MAP_SHARED and one MAP_PRIVATE.
 *
 * Build:  gcc -O2 -Wall -o mapshare mapshare.c
 * Run:    ./mapshare [file]      (default: ./mapshare.dat, created here)
 *
 * The parent (A) and the child (B) each open and mmap the file on their
 * own after the fork, the way two unrelated processes would; nothing is
 * inherited except two pipes that keep the steps in order. Each process
 * also prints its own /proc/self/maps line for the mapping.
 */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#define PAGE 4096
#define SIZE (2 * PAGE)

static const char *path = "mapshare.dat";
static int to_b[2], to_a[2]; /* pipes: A -> B and B -> A */

static void die(const char *what)
{
    perror(what);
    exit(1);
}

static void tell(int fd)
{
    if (write(fd, "x", 1) != 1)
        die("write pipe");
}

static void await(int fd)
{
    char c;
    if (read(fd, &c, 1) != 1)
        die("read pipe");
}

static void reset_file(void)
{
    char buf[SIZE];
    int fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd < 0)
        die("open");
    memset(buf, 0, sizeof buf);
    strcpy(buf, "page 0: original");
    strcpy(buf + PAGE, "page 1: original");
    if (write(fd, buf, sizeof buf) != (ssize_t)sizeof buf)
        die("write file");
    close(fd);
}

static char *map_file(int flags)
{
    int fd = open(path, O_RDWR);
    if (fd < 0)
        die("open");
    char *p = mmap(NULL, SIZE, PROT_READ | PROT_WRITE, flags, fd, 0);
    if (p == MAP_FAILED)
        die("mmap");
    close(fd); /* the mapping keeps its own reference to the file */
    return p;
}

/* What this process sees in both pages, and what read(2) of the file says. */
static void show(const char *who, const char *step, const char *m)
{
    char f[SIZE];
    int fd = open(path, O_RDONLY);
    if (fd < 0 || pread(fd, f, sizeof f, 0) != (ssize_t)sizeof f)
        die("pread");
    close(fd);
    printf("  %s %-34s map: [%-16.16s] [%-16.16s]   file: [%-16.16s] [%-16.16s]\n",
           who, step, m, m + PAGE, f, f + PAGE);
    fflush(stdout);
}

static void show_maps_line(const char *who, const char *m)
{
    char line[512];
    FILE *f = fopen("/proc/self/maps", "r");
    if (!f)
        die("/proc/self/maps");
    unsigned long start, end;
    while (fgets(line, sizeof line, f)) {
        if (sscanf(line, "%lx-%lx", &start, &end) == 2 &&
            start == (unsigned long)m) {
            printf("  %s /proc/self/maps: %s", who, line);
            break;
        }
    }
    fclose(f);
    fflush(stdout);
}

/* Round 1: both MAP_SHARED. */
static void round_shared(int is_a)
{
    char *m = map_file(MAP_SHARED);
    if (is_a) {
        show_maps_line("A", m);
        strcpy(m, "A wrote page 0");
        show("A", "after A writes page 0", m);
        tell(to_b[1]);
        await(to_a[0]);
        show("A", "after B writes page 1", m);
    } else {
        await(to_b[0]);
        show_maps_line("B", m);
        show("B", "before B writes", m);
        strcpy(m + PAGE, "B wrote page 1");
        show("B", "after B writes page 1", m);
        tell(to_a[1]);
    }
    munmap(m, SIZE);
}

/* Round 2: A is MAP_SHARED, B is MAP_PRIVATE. */
static void round_private(int is_a)
{
    if (is_a) {
        char *m = map_file(MAP_SHARED);
        await(to_a[0]); /* B has mapped and read both pages */
        strcpy(m, "A wrote page 0");
        show("A", "after A writes page 0", m);
        tell(to_b[1]);
        await(to_a[0]); /* B has written page 0 privately */
        show("A", "after B's private write", m);
        strcpy(m, "A wrote 0 again");
        strcpy(m + PAGE, "A wrote page 1");
        show("A", "after A writes both pages", m);
        tell(to_b[1]);
        await(to_a[0]);
        munmap(m, SIZE);
    } else {
        char *m = map_file(MAP_PRIVATE);
        show_maps_line("B", m);
        show("B", "after mapping (private)", m);
        tell(to_a[1]);
        await(to_b[0]);
        show("B", "after A writes page 0", m);
        strcpy(m, "B private write");
        show("B", "after B writes page 0", m);
        tell(to_a[1]);
        await(to_b[0]);
        show("B", "after A writes both pages", m);
        tell(to_a[1]);
        munmap(m, SIZE);
    }
}

int main(int argc, char **argv)
{
    if (argc > 1)
        path = argv[1];
    setvbuf(stdout, NULL, _IOLBF, 0);
    if (pipe(to_b) < 0 || pipe(to_a) < 0)
        die("pipe");

    printf("Round 1: A and B both map %s with MAP_SHARED\n", path);
    reset_file();
    pid_t pid = fork();
    if (pid < 0)
        die("fork");
    if (pid == 0) {
        round_shared(0);
        _exit(0);
    }
    round_shared(1);
    waitpid(pid, NULL, 0);

    printf("\nRound 2: A maps with MAP_SHARED, B with MAP_PRIVATE\n");
    reset_file();
    pid = fork();
    if (pid < 0)
        die("fork");
    if (pid == 0) {
        round_private(0);
        _exit(0);
    }
    round_private(1);
    waitpid(pid, NULL, 0);
    unlink(path);
    return 0;
}

// ofd.c - Lab 07, exercise G: file descriptors and open file descriptions
// on Linux.
//
// open(2): "Each open() of a file creates a new open file description",
// and a descriptor duplicated with dup(2), or inherited across fork(2),
// refers to the same one, so the two share the file offset and the file
// status flags. The descriptor flags (FD_CLOEXEC) belong to the
// descriptor. /proc/self/fdinfo shows the offset and flags of each one.
//
// usage: ./ofd [directory]       (default: the current directory)

#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>

#ifndef KCMP_FILE
#define KCMP_FILE 0
#endif

static char path[4096];

static void
die(const char *what)
{
  perror(what);
  exit(1);
}

static void
put(int fd, const char *s)
{
  if (write(fd, s, strlen(s)) != (ssize_t)strlen(s))
    die("write");
}

static void
show(const char *what)
{
  char buf[64];
  int fd = open(path, O_RDONLY);
  ssize_t n = read(fd, buf, sizeof buf - 1);
  close(fd);
  buf[n < 0 ? 0 : n] = 0;
  printf("  %-52s file: \"%s\"\n", what, buf);
}

// The pos and flags lines of /proc/self/fdinfo/<fd>, on one line.
static void
fdinfo(int fd)
{
  char p[64], line[256], out[256] = "";
  snprintf(p, sizeof p, "/proc/self/fdinfo/%d", fd);
  FILE *f = fopen(p, "r");
  if (!f) {
    printf("    fd %d: %s unreadable\n", fd, p);
    return;
  }
  while (fgets(line, sizeof line, f)) {
    line[strcspn(line, "\n")] = 0;
    if (!strncmp(line, "pos:", 4) || !strncmp(line, "flags:", 6) ||
        !strncmp(line, "ino:", 4)) {
      strcat(out, "  ");
      strcat(out, line);
    }
  }
  fclose(f);
  printf("    fdinfo/%d:%s\n", fd, out);
}

static void
kcmpfile(int a, int b)
{
  long r = syscall(SYS_kcmp, getpid(), getpid(), KCMP_FILE, a, b);
  if (r < 0)
    printf("    kcmp(KCMP_FILE, %d, %d): %s\n", a, b, strerror(errno));
  else
    printf("    kcmp(KCMP_FILE, %d, %d) = %ld (%s)\n", a, b, r,
           r == 0 ? "the same open file description" : "different ones");
}

static const char *
lockstate(int fd, int cmd, short type)
{
  struct flock fl = {.l_type = type, .l_whence = SEEK_SET};
  if (fcntl(fd, cmd, &fl) == 0)
    return "granted";
  return errno == EAGAIN || errno == EACCES ? "refused (EAGAIN)"
                                            : strerror(errno);
}

int
main(int argc, char *argv[])
{
  const char *dir = argc > 1 ? argv[1] : ".";
  snprintf(path, sizeof path, "%s/ofd.txt", dir);
  int a, b, c, d;

  printf("1. dup(): two descriptors, one open file description\n");
  a = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
  if (a < 0)
    die(path);
  b = dup(a);
  put(a, "AAAA");
  put(b, "BBBB");
  show("write AAAA via fd a, then BBBB via its dup b:");
  fcntl(a, F_SETFL, fcntl(a, F_GETFL) | O_APPEND);
  fcntl(a, F_SETFD, FD_CLOEXEC);
  printf("    after F_SETFL O_APPEND and F_SETFD FD_CLOEXEC on fd %d only:\n",
         a);
  printf("    fd %d: O_APPEND %s, FD_CLOEXEC %s\n", a,
         fcntl(a, F_GETFL) & O_APPEND ? "on" : "off",
         fcntl(a, F_GETFD) & FD_CLOEXEC ? "on" : "off");
  printf("    fd %d: O_APPEND %s, FD_CLOEXEC %s\n", b,
         fcntl(b, F_GETFL) & O_APPEND ? "on" : "off",
         fcntl(b, F_GETFD) & FD_CLOEXEC ? "on" : "off");
  fdinfo(a);
  fdinfo(b);
  kcmpfile(a, b);
  close(a);
  close(b);

  printf("2. open() twice: two open file descriptions, one inode\n");
  c = open(path, O_RDWR);
  d = open(path, O_RDWR);
  put(c, "cccc");
  put(d, "dd");
  show("write cccc via fd c, then dd via fd d:");
  fdinfo(c);
  fdinfo(d);
  kcmpfile(c, d);
  close(c);
  close(d);

  printf("3. fork(): the child's descriptor shares the parent's offset\n");
  a = open(path, O_RDWR | O_TRUNC);
  if (fork() == 0) {
    put(a, "child.");
    _exit(0);
  }
  wait(NULL);
  put(a, "parent");
  show("child writes child. and exits, then parent writes:");
  printf("    the parent's offset is now %ld\n", (long)lseek(a, 0, SEEK_CUR));

  printf("4. open(\"/proc/self/fd/%d\"): a new description or the same?\n", a);
  char self[64];
  snprintf(self, sizeof self, "/proc/self/fd/%d", a);
  c = open(self, O_RDWR);
  printf("    reopened as fd %d; its offset is %ld, fd %d's is %ld\n", c,
         (long)lseek(c, 0, SEEK_CUR), a, (long)lseek(a, 0, SEEK_CUR));
  kcmpfile(a, c);
  close(c);

  printf("5. locks: OFD locks belong to the description, POSIX locks to the "
         "process\n");
  b = dup(a);
  c = open(path, O_RDWR);
  printf("    F_OFD_SETLK write lock via fd a:              %s\n",
         lockstate(a, F_OFD_SETLK, F_WRLCK));
  printf("    F_OFD_SETLK write lock via its dup b:         %s\n",
         lockstate(b, F_OFD_SETLK, F_WRLCK));
  printf("    F_OFD_SETLK write lock via a second open() c: %s\n",
         lockstate(c, F_OFD_SETLK, F_WRLCK));
  close(a);
  printf("    after close(a), via c:                        %s\n",
         lockstate(c, F_OFD_SETLK, F_WRLCK));
  close(b);
  printf("    after close(b) too, via c:                    %s\n",
         lockstate(c, F_OFD_SETLK, F_WRLCK));
  close(c);

  // A traditional record lock is dropped when the process closes any
  // descriptor of the file, even one that had nothing to do with the lock.
  int go[2], done[2];
  if (pipe(go) < 0 || pipe(done) < 0)
    die("pipe");
  a = open(path, O_RDWR);
  printf("    F_SETLK (POSIX) write lock via fd a:          %s\n",
         lockstate(a, F_SETLK, F_WRLCK));
  fflush(stdout);
  pid_t pid = fork();
  if (pid == 0) {
    char x;
    int fd = open(path, O_RDWR);
    printf("    child, F_SETLK on its own open():             %s\n",
           lockstate(fd, F_SETLK, F_WRLCK));
    fflush(stdout);
    put(done[1], "1");
    if (read(go[0], &x, 1) != 1)
      _exit(1);
    printf("    child, again after the parent closed fd c:    %s\n",
           lockstate(fd, F_SETLK, F_WRLCK));
    fflush(stdout);
    _exit(0);
  }
  char x;
  if (read(done[0], &x, 1) != 1)
    die("read");
  c = open(path, O_RDONLY); // another descriptor, never used for locking
  close(c);
  printf("    parent opened and closed an unrelated fd c for the same file\n");
  fflush(stdout);
  put(go[1], "1");
  waitpid(pid, NULL, 0);
  close(a);

  unlink(path);
  return 0;
}

// symlinktest.c - Lab 07, exercise C: symbolic links.
//
// Needs, from your kernel change:
//   int symlink(const char *target, const char *path);   in user/user.h
//   #define T_SYMLINK 4                                   in kernel/stat.h
//   #define O_NOFOLLOW 0x800                              in kernel/fcntl.h
// open() follows a link, and a link to a link, unless O_NOFOLLOW is given,
// and fails on a cycle instead of looping. link() and unlink() act on the
// link itself. Links to directories are not required.
//
// Install: copy to user/symlinktest.c and add $U/_symlinktest to UPROGS.
// Run: symlinktest

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

static int failures;

static void
ok(int cond, char *what)
{
  printf("  %s: %s\n", cond ? "ok" : "FAILED", what);
  if (!cond)
    failures++;
}

// Open path with flags and read up to n bytes into buf as a string.
// Returns the open() result; *type gets fstat's type.
static int
slurp(char *path, int flags, char *buf, int n, int *type)
{
  struct stat st;
  int fd = open(path, flags);
  buf[0] = 0;
  *type = -1;
  if (fd < 0)
    return fd;
  int r = read(fd, buf, n - 1);
  buf[r < 0 ? 0 : r] = 0;
  fstat(fd, &st);
  *type = st.type;
  close(fd);
  return fd;
}

static void
cleanup(void)
{
  char *names[] = {"/sl.a", "/sl.b", "/sl.c", "/sl.d", "/sl.e", "/sl.x",
                   "/sl.y", "/sl.h", "/sl.1", "/sl.2", "/sl.3", "/sl.4",
                   "/sl.5", "/sl.6", "/sl.7", "/sl.8", "/sl.9", "/sl.10",
                   "/sl.11", "/sl.12", 0};
  for (int i = 0; names[i]; i++)
    unlink(names[i]);
}

int
main(void)
{
  char buf[64];
  int fd, type;

  cleanup();
  fd = open("/sl.a", O_CREATE | O_RDWR);
  write(fd, "hello from sl.a", 15);
  close(fd);

  printf("1. a link to a file\n");
  ok(symlink("/sl.a", "/sl.b") == 0, "symlink(\"/sl.a\", \"/sl.b\")");
  ok(slurp("/sl.b", O_RDONLY, buf, sizeof buf, &type) >= 0 &&
         strcmp(buf, "hello from sl.a") == 0,
     "open(\"/sl.b\") reads sl.a's contents");
  ok(type == T_FILE, "fstat() of the opened link says T_FILE: open followed it");

  printf("2. O_NOFOLLOW opens the link itself\n");
  ok(slurp("/sl.b", O_RDONLY | O_NOFOLLOW, buf, sizeof buf, &type) >= 0 &&
         type == T_SYMLINK,
     "open(\"/sl.b\", O_NOFOLLOW) gives a T_SYMLINK inode");
  printf("  its contents: \"%s\"\n", buf);

  printf("3. a chain: sl.c -> sl.b -> sl.a, and writing through it\n");
  ok(symlink("/sl.b", "/sl.c") == 0, "symlink(\"/sl.b\", \"/sl.c\")");
  fd = open("/sl.c", O_WRONLY);
  ok(fd >= 0 && write(fd, "HELLO", 5) == 5, "open(\"/sl.c\", O_WRONLY), write");
  close(fd);
  slurp("/sl.a", O_RDONLY, buf, sizeof buf, &type);
  ok(strcmp(buf, "HELLO from sl.a") == 0, "the write reached sl.a");

  printf("4. a cycle: sl.x -> sl.y -> sl.x\n");
  symlink("/sl.y", "/sl.x");
  symlink("/sl.x", "/sl.y");
  ok(open("/sl.x", O_RDONLY) < 0, "open(\"/sl.x\") fails instead of looping");

  printf("5. a chain of 12 links, and a chain of 5\n");
  char from[8], to[8];
  strcpy(to, "/sl.a");
  for (int i = 1; i <= 12; i++) {
    strcpy(from, "/sl.");
    from[4] = i < 10 ? '0' + i : '1';
    from[5] = i < 10 ? 0 : '0' + i - 10;
    from[6] = 0;
    symlink(to, from);
    strcpy(to, from);
  }
  int r12 = open("/sl.12", O_RDONLY);
  int r5 = open("/sl.5", O_RDONLY);
  printf("  open(\"/sl.12\") = %d, open(\"/sl.5\") = %d\n", r12, r5);
  ok(r5 >= 0, "5 links deep: followed");
  printf("  12 links deep: %s (where you stop is your choice; MIT suggests "
         "about 10)\n",
         r12 >= 0 ? "followed" : "refused");
  if (r12 >= 0)
    close(r12);
  if (r5 >= 0)
    close(r5);

  printf("6. a dangling link\n");
  ok(symlink("/sl.nothere", "/sl.d") == 0, "symlink to a missing file succeeds");
  ok(open("/sl.d", O_RDONLY) < 0, "open() of the dangling link fails");
  ok(open("/sl.d", O_RDONLY | O_NOFOLLOW) >= 0, "with O_NOFOLLOW it opens");

  printf("7. link() and unlink() act on the link, not the target\n");
  ok(link("/sl.b", "/sl.h") == 0, "link(\"/sl.b\", \"/sl.h\")");
  slurp("/sl.h", O_RDONLY | O_NOFOLLOW, buf, sizeof buf, &type);
  ok(type == T_SYMLINK, "sl.h is a second name for the link inode");
  ok(unlink("/sl.b") == 0, "unlink(\"/sl.b\")");
  ok(slurp("/sl.a", O_RDONLY, buf, sizeof buf, &type) >= 0,
     "sl.a is still there");
  ok(slurp("/sl.h", O_RDONLY, buf, sizeof buf, &type) >= 0 &&
         strcmp(buf, "HELLO from sl.a") == 0,
     "sl.h still leads to sl.a");
  ok(open("/sl.c", O_RDONLY) < 0, "sl.c -> sl.b now dangles");

  printf("8. a relative target (the link and the caller are both in /)\n");
  ok(symlink("sl.a", "/sl.e") == 0, "symlink(\"sl.a\", \"/sl.e\")");
  ok(slurp("/sl.e", O_RDONLY, buf, sizeof buf, &type) >= 0,
     "open(\"/sl.e\") from / works");

  cleanup();
  printf("symlinktest: %d failure(s)\n", failures);
  exit(failures);
}

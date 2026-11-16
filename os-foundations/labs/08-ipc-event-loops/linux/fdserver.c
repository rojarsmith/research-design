/*
 * fdserver.c - Stage 08, exercise A: a Unix-domain socket server that
 * receives file descriptors with SCM_RIGHTS and checks its peers with
 * SO_PEERCRED.
 *
 * Build:  gcc -O2 -Wall -o fdserver fdserver.c fdpass.c
 * Run:    ./fdserver          (forks its own clients; no input needed)
 *
 * 1. A client opens a file, moves its offset, and sends the fd. The server
 *    reads through the fd it received: same file, same offset, because
 *    both fds refer to one open file description. A second open() of the
 *    same path makes a new description with its own offset.
 * 2. A capability without a name: the client opens a file read-only,
 *    unlinks it and sends the fd. The server can still read it, cannot
 *    write through it, and shows what a reopen through /proc/self/fd does.
 * 3. SO_PEERCRED on an abstract socket, which has no file permissions, so
 *    the credential check is the only check. Run as root (as in the lab's
 *    container) it also forks clients that switch to uid 65534 before and
 *    after connect(); without root those two clients are skipped.
 * 4. Edge cases: more fds than the receiver has room for (MSG_CTRUNC), and
 *    how fds ride along with bytes on a SOCK_STREAM socket.
 */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

#include "fdpass.h"

#ifndef SO_PEERPIDFD
#define SO_PEERPIDFD 77 /* Linux 6.5 */
#endif

static char dir[64];

static void die(const char *what)
{
    perror(what);
    exit(1);
}

static socklen_t make_addr(struct sockaddr_un *a, const char *path, int abstract)
{
    memset(a, 0, sizeof *a);
    a->sun_family = AF_UNIX;
    if (abstract) {
        /* sun_path[0] == '\0': a name in the abstract namespace (unix(7)).
         * No file is created and no permissions apply; the length counts. */
        size_t n = strlen(path);
        memcpy(a->sun_path + 1, path, n);
        return (socklen_t)(offsetof(struct sockaddr_un, sun_path) + 1 + n);
    }
    snprintf(a->sun_path, sizeof a->sun_path, "%s", path);
    return (socklen_t)sizeof *a;
}

static int listen_on(const char *path, int abstract)
{
    struct sockaddr_un a;
    socklen_t len = make_addr(&a, path, abstract);
    int s = socket(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0);
    if (s < 0)
        die("socket");
    if (bind(s, (struct sockaddr *)&a, len) < 0)
        die("bind");
    if (listen(s, 8) < 0)
        die("listen");
    return s;
}

static int connect_to(const char *path, int abstract)
{
    struct sockaddr_un a;
    socklen_t len = make_addr(&a, path, abstract);
    int s = socket(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0);
    if (s < 0)
        die("socket");
    if (connect(s, (struct sockaddr *)&a, len) < 0)
        die("connect");
    return s;
}

static pid_t spawn(void (*fn)(void *), void *arg)
{
    fflush(stdout);
    pid_t pid = fork();
    if (pid < 0)
        die("fork");
    if (pid == 0) {
        fn(arg);
        fflush(stdout);
        _exit(0);
    }
    return pid;
}

static void reap(pid_t pid)
{
    int st;
    waitpid(pid, &st, 0);
}

static void say(int s, const char *msg)
{
    if (fdpass_send(s, msg, strlen(msg) + 1, NULL, 0) < 0)
        die("send");
}

static void hear(int s, char *buf, size_t len)
{
    ssize_t n = fdpass_recv(s, buf, len - 1, NULL, NULL, NULL);
    if (n < 0)
        die("recv");
    buf[n] = '\0';
}

static const char *linkname(int fd, char *buf, size_t len)
{
    char p[64];
    snprintf(p, sizeof p, "/proc/self/fd/%d", fd);
    ssize_t n = readlink(p, buf, len - 1);
    buf[n < 0 ? 0 : n] = '\0';
    return buf;
}

/* ---- part 1: one open file description, two processes ------------------ */

static char path1[128];
static const char *sock1;

static void client1(void *arg)
{
    (void)arg;
    int fd = open(path1, O_RDWR | O_CREAT | O_TRUNC | O_CLOEXEC, 0600);
    if (fd < 0)
        die("open");
    if (write(fd, "0123456789", 10) != 10)
        die("write");
    lseek(fd, 4, SEEK_SET);
    struct stat st;
    fstat(fd, &st);
    printf("  client: opened %s as fd %d, inode %lu, offset now %ld; sending it\n", path1, fd,
           (unsigned long)st.st_ino, (long)lseek(fd, 0, SEEK_CUR));
    int s = connect_to(sock1, 0);
    if (fdpass_send(s, "file", 5, &fd, 1) < 0)
        die("sendmsg");
    char buf[64];
    hear(s, buf, sizeof buf); /* the server is done reading */
    printf("  client: after the server read 3 bytes, my fd %d's offset is %ld\n", fd,
           (long)lseek(fd, 0, SEEK_CUR));
    close(s);
    close(fd);
}

static void part1(int srv)
{
    printf("1. The fd a client sends is the client's open file description\n");
    pid_t pid = spawn(client1, NULL);
    int c = accept4(srv, NULL, NULL, SOCK_CLOEXEC);
    if (c < 0)
        die("accept");
    char msg[16], name[256];
    int fd, n = 1, flags;
    if (fdpass_recv(c, msg, sizeof msg, &fd, &n, &flags) < 0 || n != 1)
        die("recvmsg");
    struct stat st;
    fstat(fd, &st);
    printf("  server: message \"%s\" with fd %d -> %s, inode %lu\n", msg, fd,
           linkname(fd, name, sizeof name), (unsigned long)st.st_ino);
    printf("  server: offset of the received fd: %ld\n", (long)lseek(fd, 0, SEEK_CUR));
    char b[4] = {0};
    if (read(fd, b, 3) != 3)
        die("read");
    printf("  server: read 3 bytes through it: \"%s\"\n", b);
    int own = open(path1, O_RDONLY | O_CLOEXEC);
    printf("  server: my own open() of the same path is fd %d, offset %ld: a new open file "
           "description\n",
           own, (long)lseek(own, 0, SEEK_CUR));
    int fl = fcntl(fd, F_GETFL);
    printf("  server: received fd status flags: %s%s\n",
           (fl & O_ACCMODE) == O_RDWR ? "O_RDWR" : (fl & O_ACCMODE) == O_RDONLY ? "O_RDONLY"
                                                                                : "O_WRONLY",
           (fl & O_APPEND) ? " | O_APPEND" : "");
    say(c, "done");
    reap(pid);
    close(own);
    close(fd);
    close(c);
    unlink(path1);
}

/* ---- part 2: a capability with no name ------------------------------- */

static void client2(void *arg)
{
    (void)arg;
    char path[160];
    snprintf(path, sizeof path, "%s/nameless.txt", dir);
    int w = open(path, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0600);
    const char text[] = "only the fd leads here";
    if (w < 0 || write(w, text, sizeof text) != (ssize_t)sizeof text)
        die("write");
    close(w);
    int ro = open(path, O_RDONLY | O_CLOEXEC);
    unlink(path); /* the file has no name any more */
    printf("  client: opened %s read-only as fd %d, then unlinked it; sending the fd\n", path,
           ro);
    int s = connect_to(sock1, 0);
    if (fdpass_send(s, "nameless", 9, &ro, 1) < 0)
        die("sendmsg");
    char buf[16];
    hear(s, buf, sizeof buf);
    close(s);
    close(ro);
}

static void part2(int srv)
{
    printf("\n2. Passing an fd passes a capability, not a name\n");
    pid_t pid = spawn(client2, NULL);
    int c = accept4(srv, NULL, NULL, SOCK_CLOEXEC);
    char msg[16], name[256], buf[64] = {0};
    int fd, n = 1;
    if (c < 0 || fdpass_recv(c, msg, sizeof msg, &fd, &n, NULL) < 0 || n != 1)
        die("recvmsg");
    printf("  server: fd %d -> %s\n", fd, linkname(fd, name, sizeof name));
    ssize_t r = pread(fd, buf, sizeof buf - 1, 0);
    printf("  server: pread -> %zd bytes: \"%s\"\n", r, buf);
    errno = 0;
    r = write(fd, "x", 1);
    printf("  server: write through it -> %zd, errno %d (%s): the access mode travelled with "
           "the fd\n",
           r, errno, strerror(errno));
    char proc[64];
    snprintf(proc, sizeof proc, "/proc/self/fd/%d", fd);
    errno = 0;
    int rw = open(proc, O_RDWR | O_CLOEXEC);
    printf("  server: open(\"%s\", O_RDWR) -> %d%s%s\n", proc, rw, rw < 0 ? ", " : "",
           rw < 0 ? strerror(errno) : "  (a reopen is checked against the file's permissions, "
                                      "not the fd's mode)");
    if (rw >= 0)
        close(rw);
    say(c, "done");
    reap(pid);
    close(fd);
    close(c);
}

/* ---- part 3: SO_PEERCRED ----------------------------------------------- */

static char abstract_name[64];

struct cred_client {
    const char *label;
    int setuid_before; /* switch to uid 65534 before connect() */
    int setuid_after;  /* ... or after connect(), before sending */
};

static void cred_client(void *arg)
{
    struct cred_client *cc = arg;
    if (cc->setuid_before && (setgid(65534) < 0 || setuid(65534) < 0))
        die("setuid");
    int s = connect_to(abstract_name, 1);
    if (cc->setuid_after && (setgid(65534) < 0 || setuid(65534) < 0))
        die("setuid");
    char msg[64], reply[96];
    snprintf(msg, sizeof msg, "%s, sent with uid %d", cc->label, (int)getuid());
    if (fdpass_send(s, msg, strlen(msg) + 1, NULL, 0) < 0) {
        printf("  client %s: send failed: %s\n", cc->label, strerror(errno));
        return;
    }
    ssize_t r = fdpass_recv(s, reply, sizeof reply - 1, NULL, NULL, NULL);
    if (r <= 0)
        printf("  client %s: %s\n", cc->label,
               r == 0 ? "server closed the connection" : strerror(errno));
    else
        printf("  client %s: server replied \"%.*s\"\n", cc->label, (int)r, reply);
    close(s);
}

static void check_peer(int srv, struct cred_client *cc)
{
    pid_t child = spawn(cred_client, cc);
    int c = accept4(srv, NULL, NULL, SOCK_CLOEXEC);
    if (c < 0)
        die("accept");
    pid_t pid;
    uid_t uid;
    gid_t gid;
    if (fdpass_peer(c, &pid, &uid, &gid) < 0)
        die("SO_PEERCRED");
    char msg[64] = "";
    ssize_t r = fdpass_recv(c, msg, sizeof msg - 1, NULL, NULL, NULL);
    msg[r < 0 ? 0 : r] = '\0';
    printf("  server: peer pid %d (fork returned %d), uid %d, gid %d; message \"%s\"\n",
           (int)pid, (int)child, (int)uid, (int)gid, msg);
    if (uid != geteuid()) {
        printf("  server: uid %d is not mine (%d): rejected, connection closed\n", (int)uid,
               (int)geteuid());
    } else {
        int pidfd = -1;
        socklen_t len = sizeof pidfd;
        if (getsockopt(c, SOL_SOCKET, SO_PEERPIDFD, &pidfd, &len) == 0) {
            char p[64], line[128];
            int fdpid;
            snprintf(p, sizeof p, "/proc/self/fdinfo/%d", pidfd);
            FILE *f = fopen(p, "r");
            while (f && fgets(line, sizeof line, f))
                if (sscanf(line, "Pid: %d", &fdpid) == 1)
                    printf("  server: SO_PEERPIDFD gave pidfd %d, whose fdinfo says Pid %d\n",
                           pidfd, fdpid);
            if (f)
                fclose(f);
            close(pidfd);
        } else {
            printf("  server: SO_PEERPIDFD: %s\n", strerror(errno));
        }
        say(c, "accepted");
    }
    close(c);
    reap(child);
}

static void part3(void)
{
    printf("\n3. SO_PEERCRED: who connected? (abstract socket, no file permissions)\n");
    snprintf(abstract_name, sizeof abstract_name, "lab08-fdserver-%d", (int)getpid());
    int srv = listen_on(abstract_name, 1);
    printf("  server: pid %d, uid %d, listening on @%s\n", (int)getpid(), (int)geteuid(),
           abstract_name);
    struct cred_client same = {"A (same uid)", 0, 0};
    check_peer(srv, &same);
    if (geteuid() == 0) {
        struct cred_client before = {"B (uid 65534 before connect)", 1, 0};
        struct cred_client after = {"C (uid 65534 after connect)", 0, 1};
        check_peer(srv, &before);
        check_peer(srv, &after);
    } else {
        printf("  (not root: the clients that switch to uid 65534 are skipped)\n");
    }
    close(srv);
}

/* ---- part 4: edge cases ------------------------------------------------ */

static void part4(void)
{
    printf("\n4. Edge cases\n");
    int sv[2];
    if (socketpair(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0, sv) < 0)
        die("socketpair");
    int three[3];
    for (int i = 0; i < 3; i++)
        three[i] = open("/dev/null", O_RDONLY | O_CLOEXEC);
    int before = fdpass_count_open();
    if (fdpass_send(sv[0], "x", 1, three, 3) < 0)
        die("sendmsg");
    int got[1], n = 1, flags = 0;
    char b[8];
    fdpass_recv(sv[1], b, sizeof b, got, &n, &flags);
    int after = fdpass_count_open();
    printf("  a) 3 fds sent, room for 1: received %d, MSG_CTRUNC %s, open fds %d -> %d\n", n,
           (flags & FDPASS_CTRUNC) ? "set" : "clear", before, after);
    fdpass_close_all(got, &n);
    for (int i = 0; i < 3; i++)
        close(three[i]);
    close(sv[0]);
    close(sv[1]);

    if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, sv) < 0)
        die("socketpair");
    int f = open("/dev/null", O_RDONLY | O_CLOEXEC);
    fdpass_send(sv[0], "AB", 2, &f, 1);
    fdpass_send(sv[0], "CD", 2, NULL, 0);
    fdpass_send(sv[0], "EF", 2, &f, 1);
    fdpass_send(sv[0], "GH", 2, NULL, 0);
    close(sv[0]);
    printf("  b) SOCK_STREAM: sent \"AB\"+fd, \"CD\", \"EF\"+fd, \"GH\"; each recvmsg asks for "
           "100 bytes and up to 4 fds\n");
    for (;;) {
        char buf[100];
        int fds[4], nf = 4;
        ssize_t r = fdpass_recv(sv[1], buf, sizeof buf, fds, &nf, NULL);
        if (r <= 0)
            break;
        printf("     recvmsg -> \"%.*s\" with %d fd(s)\n", (int)r, buf, nf);
        fdpass_close_all(fds, &nf);
    }
    close(sv[1]);
    close(f);
}

int main(void)
{
    setvbuf(stdout, NULL, _IOLBF, 0);
    snprintf(dir, sizeof dir, "/tmp/lab08-fdserver-XXXXXX");
    if (!mkdtemp(dir))
        die("mkdtemp");
    static char sockpath[128];
    snprintf(sockpath, sizeof sockpath, "%s/fdserver.sock", dir);
    snprintf(path1, sizeof path1, "%s/shared.txt", dir);
    sock1 = sockpath;
    int srv = listen_on(sockpath, 0);
    printf("server: pid %d, uid %d, listening on %s (SOCK_SEQPACKET)\n\n", (int)getpid(),
           (int)geteuid(), sockpath);
    part1(srv);
    part2(srv);
    close(srv);
    unlink(sockpath);
    part3();
    part4();
    rmdir(dir);
    return 0;
}

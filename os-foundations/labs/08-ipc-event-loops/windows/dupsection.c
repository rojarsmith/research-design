/*
 * dupsection.c - Stage 08, exercise F: hand a page-file-backed section to a
 * client with DuplicateHandle.
 *
 * Build:  gcc -O2 -Wall -o dupsection.exe dupsection.c -ladvapi32
 * Run:    dupsection.exe         (starts a copy of itself as the client)
 *         dupsection.exe hold    (the same, but stops after part 3 until you
 *                                 press Enter, so that you can look at both
 *                                 processes' handles in Process Explorer)
 *
 * 1. The server creates an unnamed 64 KiB section backed by the paging
 *    file. It has no name, so no other process can open it; the only way
 *    in is a handle that the server gives out.
 * 2. The client connects over a named pipe. The server learns its PID from
 *    the pipe, opens the client process with PROCESS_DUP_HANDLE only, and
 *    pushes two handles into it: one with FILE_MAP_READ, one with
 *    FILE_MAP_READ | FILE_MAP_WRITE. It sends the handle values over the
 *    pipe; they mean nothing in any other process.
 * 3. The client maps both, tries to map the read-only one for writing and
 *    to upgrade it with DuplicateHandle, and writes through the other.
 *    Both processes print each handle's object type, granted access and
 *    the object's handle count (NtQueryObject), which is what Process
 *    Explorer's handle view would show. The server then pulls the
 *    client's handle back and asks CompareObjectHandles whether it is the
 *    same object.
 * 4. Lifetime: the server closes every handle and view it has; the
 *    client's view still works.
 * 5. Why push and not pull: a process that holds PROCESS_DUP_HANDLE on
 *    another can duplicate that process's own pseudo-handle and get a
 *    handle to the whole process.
 */
#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0A00
#include <windows.h>
#include <sddl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SIZE (64 * 1024)

/* NtQueryObject, as documented on Learn (winternl.h); loaded at run time. */
typedef struct {
    USHORT Length, MaximumLength;
    PWSTR Buffer;
} USTR;
typedef struct {
    ULONG Attributes;
    ACCESS_MASK GrantedAccess;
    ULONG HandleCount;
    ULONG PointerCount;
    ULONG Reserved[10];
} BASIC_INFO;
typedef struct {
    USTR TypeName;
    ULONG Reserved[22];
} TYPE_INFO;
typedef LONG(NTAPI *NtQueryObject_t)(HANDLE, int, PVOID, ULONG, PULONG);
typedef BOOL(WINAPI *CompareObjectHandles_t)(HANDLE, HANDLE);

static NtQueryObject_t pNtQueryObject;

static void die(const char *what)
{
    printf("%s failed: error %lu\n", what, GetLastError());
    exit(1);
}

static void describe(const char *who, const char *what, HANDLE h)
{
    BASIC_INFO bi;
    union {
        TYPE_INFO ti;
        BYTE raw[512];
    } t;
    ULONG len;
    memset(&bi, 0, sizeof bi);
    memset(&t, 0, sizeof t);
    pNtQueryObject(h, 0 /* ObjectBasicInformation */, &bi, sizeof bi, &len);
    pNtQueryObject(h, 2 /* ObjectTypeInformation */, &t, sizeof t, &len);
    printf("  %s: %-28s handle 0x%04llx  type %-8.*ls access 0x%08lx  handle count %lu\n", who,
           what, (unsigned long long)(ULONG_PTR)h, t.ti.TypeName.Length / 2,
           t.ti.TypeName.Buffer ? t.ti.TypeName.Buffer : L"?", bi.GrantedAccess, bi.HandleCount);
}

static void pipe_send(HANDLE p, const char *s)
{
    DWORD w;
    if (!WriteFile(p, s, (DWORD)strlen(s) + 1, &w, NULL))
        die("WriteFile");
}

static void pipe_recv(HANDLE p, char *buf, DWORD len)
{
    DWORD r;
    if (!ReadFile(p, buf, len - 1, &r, NULL))
        die("ReadFile");
    buf[r] = 0;
}

/* ------------------------------------------------------------------------ */

static int client(const char *pipename)
{
    HANDLE p = CreateFileA(pipename, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
                           SECURITY_SQOS_PRESENT | SECURITY_IDENTIFICATION, NULL);
    if (p == INVALID_HANDLE_VALUE)
        die("client CreateFile");
    DWORD mode = PIPE_READMODE_MESSAGE;
    SetNamedPipeHandleState(p, &mode, NULL, NULL);
    pipe_send(p, "hello");

    char msg[128];
    unsigned long long ro_v, rw_v, lk_v;
    pipe_recv(p, msg, sizeof msg);
    if (sscanf(msg, "RO %llx RW %llx LOCKED %llx", &ro_v, &rw_v, &lk_v) != 3)
        die("bad message");
    HANDLE ro = (HANDLE)(ULONG_PTR)ro_v, rw = (HANDLE)(ULONG_PTR)rw_v,
           lk = (HANDLE)(ULONG_PTR)lk_v;
    printf("  client: the server says my handles are 0x%04llx (read-only) and 0x%04llx "
           "(read-write)\n",
           ro_v, rw_v);
    describe("client", "read-only handle", ro);
    describe("client", "read-write handle", rw);

    const char *view = MapViewOfFile(ro, FILE_MAP_READ, 0, 0, 0);
    if (!view)
        die("MapViewOfFile(read)");
    MEMORY_BASIC_INFORMATION mbi;
    VirtualQuery(view, &mbi, sizeof mbi);
    printf("  client: read-only view at %p, %llu bytes, %s: \"%s\"\n", (void *)view,
           (unsigned long long)mbi.RegionSize, mbi.Type == MEM_MAPPED ? "MEM_MAPPED" : "?", view);
    void *w = MapViewOfFile(ro, FILE_MAP_WRITE, 0, 0, 0);
    printf("  client: MapViewOfFile(read-only handle, FILE_MAP_WRITE) -> %s, error %lu\n",
           w ? "a view" : "NULL", w ? 0 : GetLastError());
    /* Can the client get more access than it was given? DuplicateHandle
     * with a new access mask asks the object's security descriptor again. */
    HANDLE up = NULL;
    BOOL ok = DuplicateHandle(GetCurrentProcess(), ro, GetCurrentProcess(), &up,
                              FILE_MAP_ALL_ACCESS, FALSE, 0);
    printf("  client: DuplicateHandle(read-only handle, FILE_MAP_ALL_ACCESS) -> %s, error %lu\n",
           ok ? "TRUE" : "FALSE", ok ? 0 : GetLastError());
    if (ok) {
        describe("client", "upgraded copy", up);
        char *uv = MapViewOfFile(up, FILE_MAP_WRITE, 0, 0, 0);
        printf("  client: MapViewOfFile(upgraded copy, FILE_MAP_WRITE) -> %s\n",
               uv ? "a writable view" : "NULL");
        if (uv)
            UnmapViewOfFile(uv);
        CloseHandle(up);
    }
    describe("client", "locked-down section handle", lk);
    up = NULL;
    ok = DuplicateHandle(GetCurrentProcess(), lk, GetCurrentProcess(), &up, FILE_MAP_ALL_ACCESS,
                         FALSE, 0);
    printf("  client: DuplicateHandle(locked-down handle, FILE_MAP_ALL_ACCESS) -> %s, error %lu\n",
           ok ? "TRUE" : "FALSE", ok ? 0 : GetLastError());
    if (ok)
        CloseHandle(up);
    const char *lv = MapViewOfFile(lk, FILE_MAP_READ, 0, 0, 0);
    printf("  client: its read-only view says \"%s\"\n", lv ? lv : "(map failed)");
    if (lv)
        UnmapViewOfFile(lv);
    CloseHandle(lk);

    char *wv = MapViewOfFile(rw, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
    if (!wv)
        die("MapViewOfFile(write)");
    strcpy(wv + 4096, "page 1, written by the client");
    printf("  client: wrote into page 1 through the read-write handle's view at %p\n", (void *)wv);
    UnmapViewOfFile(wv);
    CloseHandle(rw);
    pipe_send(p, "written");

    pipe_recv(p, msg, sizeof msg); /* the server has let go of everything */
    CloseHandle(ro);
    printf("  client: closed my handles too; my view still reads \"%s\" and \"%s\"\n", view,
           view + 4096);
    pipe_send(p, "lifetime done");

    /* part 5: what PROCESS_DUP_HANDLE on the server would give */
    pipe_recv(p, msg, sizeof msg);
    ULONG spid = 0;
    GetNamedPipeServerProcessId(p, &spid);
    HANDLE srv = OpenProcess(PROCESS_DUP_HANDLE, FALSE, spid);
    HANDLE full = NULL;
    if (srv && DuplicateHandle(srv, (HANDLE)(LONG_PTR)-1 /* the server's GetCurrentProcess() */,
                               GetCurrentProcess(), &full, 0, FALSE, DUPLICATE_SAME_ACCESS)) {
        describe("client", "server's pseudo-handle, copied", full);
        CloseHandle(full);
    } else {
        printf("  client: pulling the server's pseudo-handle failed: error %lu\n", GetLastError());
    }
    if (srv)
        CloseHandle(srv);
    UnmapViewOfFile(view);
    pipe_send(p, "bye");
    CloseHandle(p);
    return 0;
}

int main(int argc, char **argv)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    pNtQueryObject = (NtQueryObject_t)(void *)GetProcAddress(GetModuleHandleA("ntdll.dll"),
                                                             "NtQueryObject");
    CompareObjectHandles_t pCompare = (CompareObjectHandles_t)(void *)GetProcAddress(
        GetModuleHandleA("kernelbase.dll"), "CompareObjectHandles");
    if (!pNtQueryObject)
        die("GetProcAddress(NtQueryObject)");
    if (argc >= 3 && strcmp(argv[1], "client") == 0)
        return client(argv[2]);
    int hold = argc >= 2 && strcmp(argv[1], "hold") == 0;

    printf("1. An unnamed section backed by the paging file\n");
    HANDLE sec = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, SIZE, NULL);
    if (!sec)
        die("CreateFileMapping");
    char *view = MapViewOfFile(sec, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
    if (!view)
        die("MapViewOfFile");
    strcpy(view, "page 0, written by the server");
    printf("  server: pid %lu, section of %d bytes, view at %p\n", GetCurrentProcessId(), SIZE,
           (void *)view);
    describe("server", "section handle", sec);

    printf("\n2. Push two handles into the client\n");
    char pipename[64];
    snprintf(pipename, sizeof pipename, "\\\\.\\pipe\\lab08-dupsection-%lu",
             GetCurrentProcessId());
    HANDLE p = CreateNamedPipeA(pipename, PIPE_ACCESS_DUPLEX | FILE_FLAG_FIRST_PIPE_INSTANCE,
                                PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT |
                                    PIPE_REJECT_REMOTE_CLIENTS,
                                1, 4096, 4096, 0, NULL);
    if (p == INVALID_HANDLE_VALUE)
        die("CreateNamedPipe");
    char self[MAX_PATH], cmd[512];
    GetModuleFileNameA(NULL, self, MAX_PATH);
    snprintf(cmd, sizeof cmd, "\"%s\" client %s", self, pipename);
    STARTUPINFOA si = {.cb = sizeof si};
    PROCESS_INFORMATION pi;
    if (!CreateProcessA(self, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
        die("CreateProcess");
    if (!ConnectNamedPipe(p, NULL) && GetLastError() != ERROR_PIPE_CONNECTED)
        die("ConnectNamedPipe");
    char msg[128];
    pipe_recv(p, msg, sizeof msg);
    ULONG cpid = 0;
    GetNamedPipeClientProcessId(p, &cpid);
    printf("  server: client pid %lu connected (CreateProcess said %lu)\n", cpid,
           pi.dwProcessId);
    HANDLE cproc = OpenProcess(PROCESS_DUP_HANDLE, FALSE, cpid);
    if (!cproc)
        die("OpenProcess(PROCESS_DUP_HANDLE)");
    HANDLE ro = NULL, rw = NULL;
    if (!DuplicateHandle(GetCurrentProcess(), sec, cproc, &ro, FILE_MAP_READ, FALSE, 0) ||
        !DuplicateHandle(GetCurrentProcess(), sec, cproc, &rw, FILE_MAP_READ | FILE_MAP_WRITE,
                         FALSE, 0))
        die("DuplicateHandle");
    printf("  server: DuplicateHandle into pid %lu -> 0x%04llx (FILE_MAP_READ), 0x%04llx "
           "(FILE_MAP_READ | FILE_MAP_WRITE); valid only in that process\n",
           cpid, (unsigned long long)(ULONG_PTR)ro, (unsigned long long)(ULONG_PTR)rw);
    describe("server", "section handle", sec);

    /* A second section whose security descriptor grants this user only
     * SECTION_QUERY | SECTION_MAP_READ (0x5), with protected inheritance. */
    HANDLE tok;
    BYTE ubuf[256];
    DWORD len;
    LPSTR ssid = NULL;
    OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &tok);
    GetTokenInformation(tok, TokenUser, ubuf, sizeof ubuf, &len);
    CloseHandle(tok);
    ConvertSidToStringSidA(((TOKEN_USER *)ubuf)->User.Sid, &ssid);
    char sddl[256];
    snprintf(sddl, sizeof sddl, "D:P(A;;0x5;;;%s)", ssid);
    LocalFree(ssid);
    SECURITY_ATTRIBUTES sa = {.nLength = sizeof sa};
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorA(sddl, SDDL_REVISION_1,
                                                              &sa.lpSecurityDescriptor, NULL))
        die("ConvertStringSecurityDescriptorToSecurityDescriptor");
    HANDLE locked = CreateFileMappingW(INVALID_HANDLE_VALUE, &sa, PAGE_READWRITE, 0, 4096, NULL);
    LocalFree(sa.lpSecurityDescriptor);
    if (!locked)
        die("CreateFileMapping(locked)");
    char *lv = MapViewOfFile(locked, FILE_MAP_WRITE, 0, 0, 0);
    if (lv) {
        strcpy(lv, "the locked-down section");
        UnmapViewOfFile(lv);
    }
    describe("server", "locked-down section (DACL 0x5)", locked);
    HANDLE lk = NULL;
    if (!DuplicateHandle(GetCurrentProcess(), locked, cproc, &lk, FILE_MAP_READ, FALSE, 0))
        die("DuplicateHandle(locked)");
    printf("  server: pushed a FILE_MAP_READ handle to it as 0x%04llx\n",
           (unsigned long long)(ULONG_PTR)lk);
    snprintf(msg, sizeof msg, "RO %llx RW %llx LOCKED %llx", (unsigned long long)(ULONG_PTR)ro,
             (unsigned long long)(ULONG_PTR)rw, (unsigned long long)(ULONG_PTR)lk);
    pipe_send(p, msg);

    printf("\n3. The client maps them\n");
    pipe_recv(p, msg, sizeof msg); /* "written" */
    printf("  server: page 1 of my view now reads \"%s\"\n", view + 4096);
    HANDLE back = NULL, other = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0,
                                                   4096, NULL);
    if (DuplicateHandle(cproc, ro, GetCurrentProcess(), &back, 0, FALSE, DUPLICATE_SAME_ACCESS) &&
        pCompare) {
        printf("  server: pulled the client's 0x%04llx back as 0x%04llx; CompareObjectHandles "
               "with my section: %s; with another section: %s\n",
               (unsigned long long)(ULONG_PTR)ro, (unsigned long long)(ULONG_PTR)back,
               pCompare(sec, back) ? "TRUE" : "FALSE", pCompare(other, back) ? "TRUE" : "FALSE");
        CloseHandle(back);
    }
    CloseHandle(other);
    CloseHandle(locked);

    if (hold) {
        /* Both processes are alive now: the client waits on the pipe. */
        printf("\n  [hold] server pid %lu, client pid %lu: look at both in Process Explorer "
               "(View > Lower Pane View > Handles), then press Enter here\n",
               GetCurrentProcessId(), cpid);
        getchar();
    }

    printf("\n4. Lifetime\n");
    UnmapViewOfFile(view);
    CloseHandle(sec);
    printf("  server: unmapped my view and closed my section handle\n");
    pipe_send(p, "released");
    pipe_recv(p, msg, sizeof msg); /* "lifetime done" */

    printf("\n5. Push, do not pull\n");
    pipe_send(p, "go");
    pipe_recv(p, msg, sizeof msg); /* "bye" */
    WaitForSingleObject(pi.hProcess, 10000);
    DWORD code = 0;
    GetExitCodeProcess(pi.hProcess, &code);
    printf("  server: client exited with code %lu\n", code);
    CloseHandle(cproc);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(p);
    return 0;
}

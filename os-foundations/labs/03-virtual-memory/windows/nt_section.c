/*
 * nt_section.c - Stage 03, exercise G: file mapping objects (sections) on NT.
 *
 * Build:  gcc -O2 -Wall -o nt_section.exe nt_section.c -lpsapi
 * Run:    nt_section.exe
 *
 * Part 1: a page-file-backed section created with a name, opened by a
 *         second process (this program started again with --child), and
 *         written by both. The child also maps a FILE_MAP_COPY view, NT's
 *         counterpart of MAP_PRIVATE, and reports each page's protection.
 * Part 2: the system commit charge of a 256 MiB section created with
 *         SEC_COMMIT (the default) and with SEC_RESERVE.
 * Part 3: try to shrink a data file while a view of it is mapped: NT's
 *         answer to the case that gives SIGBUS on Linux.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

#define PAGE 4096
#define MiB (1024.0 * 1024.0)

static double commit_total_mib(void)
{
    PERFORMANCE_INFORMATION pi;
    pi.cb = sizeof pi;
    GetPerformanceInfo(&pi, sizeof pi);
    return (double)pi.CommitTotal * pi.PageSize / MiB;
}

static double private_mib(void)
{
    PROCESS_MEMORY_COUNTERS_EX pmc;
    pmc.cb = sizeof pmc;
    GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS *)&pmc,
                         sizeof pmc);
    return pmc.PrivateUsage / MiB;
}

static const char *prot_name(DWORD p)
{
    switch (p & 0xff) {
    case PAGE_READONLY: return "PAGE_READONLY";
    case PAGE_READWRITE: return "PAGE_READWRITE";
    case PAGE_WRITECOPY: return "PAGE_WRITECOPY";
    case PAGE_NOACCESS: return "PAGE_NOACCESS";
    default: return "other";
    }
}

static void show(const char *who, const char *step, const char *v)
{
    printf("  %s %-30s view %p: [%-20.20s] [%-20.20s] [%-20.20s]\n", who, step,
           (void *)v, v, v + PAGE, v + 2 * PAGE);
}

/* ---- the child process ------------------------------------------------ */

static int child(const wchar_t *name)
{
    HANDLE h = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, name);
    if (!h) {
        printf("  child: OpenFileMapping failed: %lu\n", GetLastError());
        return 1;
    }
    char *v = MapViewOfFile(h, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if (!v) {
        printf("  child: MapViewOfFile failed: %lu\n", GetLastError());
        return 1;
    }
    show("B", "after opening by name", v);
    strcpy(v + PAGE, "B wrote page 1");
    show("B", "after B writes page 1", v);

    double before = private_mib();
    char *c = MapViewOfFile(h, FILE_MAP_COPY, 0, 0, 3 * PAGE);
    if (!c) {
        printf("  child: FILE_MAP_COPY view failed: %lu\n", GetLastError());
        return 1;
    }
    double after = private_mib();
    show("B", "copy view, before writing", c);
    strcpy(c, "B private write");
    show("B", "copy view, after writing 0", c);
    show("B", "shared view, same moment", v);
    printf("  B commit charge: %+.3f MiB for mapping a 3-page copy view\n",
           after - before);
    for (int i = 0; i < 3; i++) {
        MEMORY_BASIC_INFORMATION mbi;
        VirtualQuery(c + i * PAGE, &mbi, sizeof mbi);
        PSAPI_WORKING_SET_EX_INFORMATION ws;
        ws.VirtualAddress = c + i * PAGE;
        QueryWorkingSetEx(GetCurrentProcess(), &ws, sizeof ws);
        printf("  B copy view page %d: %-15s %s\n", i, prot_name(mbi.Protect),
               ws.VirtualAttributes.Valid
                   ? (ws.VirtualAttributes.Shared ? "resident, shared"
                                                  : "resident, private")
                   : "not in the working set");
    }
    UnmapViewOfFile(c);
    UnmapViewOfFile(v);
    CloseHandle(h);
    return 0;
}

/* ---- part 1 ------------------------------------------------------------ */

static void part1(const wchar_t *self)
{
    wchar_t name[64];
    swprintf(name, 64, L"Local\\oslab03-%lu", GetCurrentProcessId());
    printf("1. A page-file-backed section named %ls, shared by two processes\n",
           name);
    HANDLE h = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0,
                                  1 << 20, name);
    if (!h) {
        printf("  CreateFileMapping failed: %lu\n", GetLastError());
        return;
    }
    char *v = MapViewOfFile(h, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    strcpy(v, "A wrote page 0");
    show("A", "after A writes page 0", v);

    STARTUPINFOW si = {0};
    PROCESS_INFORMATION pi;
    wchar_t cmd[2 * MAX_PATH];
    si.cb = sizeof si;
    swprintf(cmd, 2 * MAX_PATH, L"\"%ls\" --child %ls", self, name);
    if (!CreateProcessW(self, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        printf("  CreateProcess failed: %lu\n", GetLastError());
        return;
    }
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD code;
    GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    show("A", "after B exited", v);
    printf("  child exit code %lu\n", code);
    UnmapViewOfFile(v);
    CloseHandle(h);
}

/* ---- part 2 ------------------------------------------------------------ */

static void part2(void)
{
    const DWORD size = 256u << 20;
    printf("\n2. System commit charge for a 256 MiB page-file-backed section\n");
    for (int r = 0; r < 2; r++) {
        DWORD attr = r == 0 ? SEC_COMMIT : SEC_RESERVE;
        double c0 = commit_total_mib();
        HANDLE h = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE | attr,
                                      0, size, NULL);
        double c1 = commit_total_mib();
        char *v = MapViewOfFile(h, FILE_MAP_ALL_ACCESS, 0, 0, 0);
        double c2 = commit_total_mib();
        if (r == 1)
            VirtualAlloc(v, 16 << 20, MEM_COMMIT, PAGE_READWRITE);
        double c3 = commit_total_mib();
        printf("  %-11s create %+7.1f MiB, map a view %+7.1f MiB, %s %+7.1f MiB\n",
               r == 0 ? "SEC_COMMIT" : "SEC_RESERVE", c1 - c0, c2 - c1,
               r == 0 ? "(nothing more)       " : "VirtualAlloc 16 MiB  ", c3 - c2);
        UnmapViewOfFile(v);
        CloseHandle(h);
    }
    printf("  (the system total also moves with every other process; run it twice)\n");
}

/* ---- part 3 ------------------------------------------------------------ */

static void try_truncate(const char *when, HANDLE f)
{
    LARGE_INTEGER pos;
    pos.QuadPart = PAGE;
    SetFilePointerEx(f, pos, NULL, FILE_BEGIN);
    BOOL ok = SetEndOfFile(f);
    DWORD err = ok ? 0 : GetLastError();
    char msg[256] = "";
    if (!ok)
        FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                       NULL, err, 0, msg, sizeof msg, NULL);
    msg[strcspn(msg, "\r\n")] = 0;
    LARGE_INTEGER sz;
    GetFileSizeEx(f, &sz);
    printf("  %-44s SetEndOfFile -> %s", when, ok ? "TRUE" : "FALSE");
    if (!ok)
        printf(", error %lu (%s)", err, msg);
    printf(", file size now %lld\n", sz.QuadPart);
}

static void part3(void)
{
    wchar_t dir[MAX_PATH], path[MAX_PATH];
    GetTempPathW(MAX_PATH, dir);
    swprintf(path, MAX_PATH, L"%lsoslab03-%lu.dat", dir, GetCurrentProcessId());
    printf("\n3. Shrink a mapped data file from 3 pages to 1\n");
    HANDLE f = CreateFileW(path, GENERIC_READ | GENERIC_WRITE,
                           FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,
                           FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, NULL);
    if (f == INVALID_HANDLE_VALUE) {
        printf("  CreateFile failed: %lu\n", GetLastError());
        return;
    }
    char buf[3 * PAGE];
    memset(buf, 'x', sizeof buf);
    DWORD n;
    WriteFile(f, buf, sizeof buf, &n, NULL);
    HANDLE m = CreateFileMappingW(f, NULL, PAGE_READWRITE, 0, 0, NULL);
    char *v = MapViewOfFile(m, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    printf("  mapped %lu bytes; byte at page 2 reads '%c'\n", n, v[2 * PAGE]);

    /* a second, independent handle, as another process would have */
    HANDLE f2 = CreateFileW(path, GENERIC_READ | GENERIC_WRITE,
                            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                            NULL, OPEN_EXISTING, 0, NULL);
    try_truncate("with a view mapped (same handle):", f);
    if (f2 != INVALID_HANDLE_VALUE)
        try_truncate("with a view mapped (second handle):", f2);
    UnmapViewOfFile(v);
    try_truncate("view unmapped, section handle still open:", f);
    CloseHandle(m);
    try_truncate("view unmapped and section closed:", f);
    if (f2 != INVALID_HANDLE_VALUE)
        CloseHandle(f2);
    CloseHandle(f);
}

int wmain(int argc, wchar_t **argv)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    if (argc == 3 && wcscmp(argv[1], L"--child") == 0)
        return child(argv[2]);
    wchar_t self[MAX_PATH];
    GetModuleFileNameW(NULL, self, MAX_PATH);
    part1(self);
    part2();
    part3();
    return 0;
}

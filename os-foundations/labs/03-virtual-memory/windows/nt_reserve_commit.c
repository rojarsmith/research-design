/*
 * nt_reserve_commit.c - Stage 03, exercise G: reserve, commit, touch,
 * decommit and release on NT, and what each step costs in commit charge
 * and in working set.
 *
 * Build:  gcc -O2 -Wall -o nt_reserve_commit.exe nt_reserve_commit.c -lpsapi
 * Run:    nt_reserve_commit.exe
 *
 * Part 1 walks one region through the page states (free, reserved,
 * committed) and prints, after each step:
 *   - this process's commit charge (PROCESS_MEMORY_COUNTERS_EX.PrivateUsage),
 *   - the system's commit total (PERFORMANCE_INFORMATION.CommitTotal),
 *   - this process's working set and page-fault count,
 *   - the region's state as VirtualQuery reports it.
 * Other processes also change the system commit total while this runs.
 *
 * Part 2 commits pages on demand from a vectored exception handler, the
 * way Learn's "Reserving and Committing Memory" example does with an
 * exception filter. It is NT's version of xv6's lazy sbrk: reserve the
 * range, take the fault, back the page, retry the instruction.
 *
 * Part 3 lists this process's address space by allocation, the NT
 * counterpart of /proc/self/maps.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#include <stdio.h>
#include <string.h>

#define MiB (1024.0 * 1024.0)

static SIZE_T g_page;

struct snap {
    SIZE_T private_usage, working_set, commit_total;
    DWORD faults;
};

static void take(struct snap *s)
{
    PROCESS_MEMORY_COUNTERS_EX pmc;
    PERFORMANCE_INFORMATION pi;
    pmc.cb = sizeof pmc;
    GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS *)&pmc,
                         sizeof pmc);
    pi.cb = sizeof pi;
    GetPerformanceInfo(&pi, sizeof pi);
    s->private_usage = pmc.PrivateUsage;
    s->working_set = pmc.WorkingSetSize;
    s->faults = pmc.PageFaultCount;
    s->commit_total = pi.CommitTotal * pi.PageSize;
}

static const char *state_name(DWORD s)
{
    switch (s) {
    case MEM_COMMIT: return "MEM_COMMIT";
    case MEM_RESERVE: return "MEM_RESERVE";
    case MEM_FREE: return "MEM_FREE";
    default: return "?";
    }
}

static const char *type_name(DWORD t)
{
    switch (t) {
    case MEM_IMAGE: return "MEM_IMAGE";
    case MEM_MAPPED: return "MEM_MAPPED";
    case MEM_PRIVATE: return "MEM_PRIVATE";
    default: return "-";
    }
}

static void row(const char *step, const struct snap *base, void *addr)
{
    struct snap s;
    take(&s);
    char where[80] = "";
    if (addr) {
        MEMORY_BASIC_INFORMATION mbi;
        VirtualQuery(addr, &mbi, sizeof mbi);
        snprintf(where, sizeof where, "%s, %.0f MiB run", state_name(mbi.State),
                 mbi.RegionSize / MiB);
    }
    printf("  %-34s %+9.1f %+9.1f %+9.1f %+8ld   %s\n", step,
           ((double)s.private_usage - (double)base->private_usage) / MiB,
           ((double)s.commit_total - (double)base->commit_total) / MiB,
           ((double)s.working_set - (double)base->working_set) / MiB,
           (long)s.faults - (long)base->faults, where);
}

static void part1(void)
{
    const SIZE_T reserve = (SIZE_T)1 << 30, commit = (SIZE_T)256 << 20;
    struct snap base;

    printf("1. One region through its states (MiB and faults relative to the start)\n");
    printf("  %-34s %9s %9s %9s %8s   %s\n", "step", "process", "system", "working",
           "page", "VirtualQuery at the base");
    printf("  %-34s %9s %9s %9s %8s\n", "", "commit", "commit", "set", "faults");
    take(&base);
    row("start", &base, NULL);
    char *p = VirtualAlloc(NULL, reserve, MEM_RESERVE, PAGE_READWRITE);
    if (!p) {
        printf("  VirtualAlloc(MEM_RESERVE) failed: %lu\n", GetLastError());
        return;
    }
    row("reserve 1 GiB", &base, p);
    if (!VirtualAlloc(p, commit, MEM_COMMIT, PAGE_READWRITE)) {
        printf("  VirtualAlloc(MEM_COMMIT) failed: %lu\n", GetLastError());
        return;
    }
    row("commit the first 256 MiB", &base, p);
    for (SIZE_T i = 0; i < commit; i += g_page)
        p[i] = 1;
    row("write one byte in each page", &base, p);
    VirtualFree(p, commit, MEM_DECOMMIT);
    row("decommit the 256 MiB", &base, p);
    VirtualFree(p, 0, MEM_RELEASE);
    row("release the region", &base, p);
}

/* ---- part 2: commit on demand ----------------------------------------- */

static char *g_lazy;
static SIZE_T g_lazy_size;
static volatile LONG g_commits;

static LONG CALLBACK commit_on_fault(EXCEPTION_POINTERS *ep)
{
    EXCEPTION_RECORD *er = ep->ExceptionRecord;
    if (er->ExceptionCode != EXCEPTION_ACCESS_VIOLATION || er->NumberParameters < 2)
        return EXCEPTION_CONTINUE_SEARCH;
    char *addr = (char *)er->ExceptionInformation[1];
    if (addr < g_lazy || addr >= g_lazy + g_lazy_size)
        return EXCEPTION_CONTINUE_SEARCH; /* not ours: let it crash */
    if (!VirtualAlloc(addr, 1, MEM_COMMIT, PAGE_READWRITE))
        return EXCEPTION_CONTINUE_SEARCH;
    InterlockedIncrement(&g_commits);
    return EXCEPTION_CONTINUE_EXECUTION; /* retry the faulting instruction */
}

static void lazy_writes(SIZE_T stride, const char *label)
{
    struct snap base, s;

    g_lazy_size = (SIZE_T)64 << 20;
    g_lazy = VirtualAlloc(NULL, g_lazy_size, MEM_RESERVE, PAGE_READWRITE);
    if (!g_lazy) {
        printf("  VirtualAlloc failed: %lu\n", GetLastError());
        return;
    }
    g_commits = 0;
    take(&base);
    for (int i = 0; i < 16; i++)
        g_lazy[(SIZE_T)i * stride + 123] = (char)i;
    take(&s);
    int ok = 1;
    for (int i = 0; i < 16; i++)
        if (g_lazy[(SIZE_T)i * stride + 123] != (char)i)
            ok = 0;
    printf("  16 pages %-13s handler committed %2ld pages; commit charge %+5.0f KiB "
           "(%2zu pages); read back: %s\n",
           label, g_commits,
           ((double)s.private_usage - (double)base.private_usage) / 1024.0,
           (s.private_usage - base.private_usage) / g_page, ok ? "ok" : "WRONG");
    if (stride > g_page) {
        MEMORY_BASIC_INFORMATION mbi;
        VirtualQuery(g_lazy + g_page, &mbi, sizeof mbi);
        printf("  %-23s the page after the first is still %s (a %.2f MiB run)\n", "",
               state_name(mbi.State), mbi.RegionSize / MiB);
    }
    VirtualFree(g_lazy, 0, MEM_RELEASE);
}

static void part2(void)
{
    printf("\n2. Commit on demand: reserve 64 MiB, write 16 pages, commit each "
           "page in the fault handler\n");
    PVOID h = AddVectoredExceptionHandler(1, commit_on_fault);
    lazy_writes((SIZE_T)4 << 20, "4 MiB apart:");
    lazy_writes(g_page, "side by side:");
    RemoveVectoredExceptionHandler(h);
}

/* ---- part 3: the address space, by allocation ------------------------- */

static void part3(void)
{
    MEMORY_BASIC_INFORMATION mbi;
    char *a = NULL;
    int nprivate = 0, nimage = 0, nmapped = 0;
    double private_mib = 0, image_mib = 0, mapped_mib = 0;

    printf("\n3. This process's address space, one line per image or mapped "
           "allocation\n");
    printf("  %-18s %10s  %-11s %s\n", "allocation base", "size KiB", "type", "file");
    while (VirtualQuery(a, &mbi, sizeof mbi) == sizeof mbi) {
        if (mbi.State != MEM_FREE && mbi.AllocationBase == mbi.BaseAddress) {
            /* add up every region of this allocation */
            char *base = mbi.AllocationBase, *b = base;
            SIZE_T total = 0;
            MEMORY_BASIC_INFORMATION m2;
            while (VirtualQuery(b, &m2, sizeof m2) == sizeof m2 &&
                   m2.AllocationBase == base) {
                total += m2.RegionSize;
                b += m2.RegionSize;
            }
            if (mbi.Type == MEM_PRIVATE) {
                nprivate++;
                private_mib += total / MiB;
            } else {
                wchar_t name[MAX_PATH] = L"";
                GetMappedFileNameW(GetCurrentProcess(), base, name, MAX_PATH);
                const wchar_t *shortname = wcsrchr(name, L'\\');
                printf("  %p %10.0f  %-11s %ls\n", (void *)base, total / 1024.0,
                       type_name(mbi.Type), shortname ? shortname + 1 : name);
                if (mbi.Type == MEM_IMAGE) {
                    nimage++;
                    image_mib += total / MiB;
                } else {
                    nmapped++;
                    mapped_mib += total / MiB;
                }
            }
        }
        a = (char *)mbi.BaseAddress + mbi.RegionSize;
    }
    printf("  totals: %d image allocations (%.1f MiB), %d mapped (%.1f MiB), "
           "%d private (%.1f MiB, reserved or committed)\n",
           nimage, image_mib, nmapped, mapped_mib, nprivate, private_mib);
}

int main(void)
{
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    g_page = si.dwPageSize;
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("Page size %lu bytes, allocation granularity %lu KiB\n\n", si.dwPageSize,
           si.dwAllocationGranularity / 1024);
    part1();
    part2();
    part3();
    return 0;
}

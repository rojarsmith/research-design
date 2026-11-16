/*
 * nt_intr_rates.c - Stage 04, exercise F: interrupt and DPC rates on NT,
 * read through the Performance Data Helper (PDH) without administrator
 * rights.
 *
 * Build:  gcc -O2 -Wall -o nt_intr_rates.exe nt_intr_rates.c -lpdh -lwinmm
 * Run:    nt_intr_rates.exe [seconds per phase, default 3]
 *
 * It samples the Processor counters once per phase:
 *   Interrupts/sec, DPCs Queued/sec, % Interrupt Time, % DPC Time
 * for _Total and for each logical processor, in three phases:
 *   a) quiet: this program only waits;
 *   b) timer: a thread holds timeBeginPeriod(1) and loops on Sleep(1);
 *   c) quiet again.
 * The counter paths are the English names (PdhAddEnglishCounterW), so the
 * program also works on a Windows installed in another language.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <timeapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

#define NCOUNTERS 4
static const wchar_t *names[NCOUNTERS] = {
    L"Interrupts/sec", L"DPCs Queued/sec", L"% Interrupt Time", L"% DPC Time"};

static PDH_HQUERY q;
static PDH_HCOUNTER total[NCOUNTERS], percpu_intr;
static volatile LONG stop;
static volatile LONG sleeps;

static DWORD WINAPI timer_thread(LPVOID arg)
{
    (void)arg;
    timeBeginPeriod(1);
    while (!stop) {
        Sleep(1);
        InterlockedIncrement(&sleeps);
    }
    timeEndPeriod(1);
    return 0;
}

static double value(PDH_HCOUNTER c)
{
    PDH_FMT_COUNTERVALUE v;
    if (PdhGetFormattedCounterValue(c, PDH_FMT_DOUBLE | PDH_FMT_NOCAP100, NULL, &v) !=
            ERROR_SUCCESS ||
        v.CStatus != PDH_CSTATUS_VALID_DATA)
        return -1;
    return v.doubleValue;
}

static void phase(const char *label, int seconds)
{
    PdhCollectQueryData(q);
    Sleep(seconds * 1000);
    PdhCollectQueryData(q);
    printf("  %-34s", label);
    for (int i = 0; i < NCOUNTERS; i++)
        printf(" %12.1f", value(total[i]));
    printf("\n");

    DWORD size = 0, count = 0;
    if (PdhGetFormattedCounterArrayW(percpu_intr, PDH_FMT_DOUBLE, &size, &count, NULL) ==
        PDH_MORE_DATA) {
        PDH_FMT_COUNTERVALUE_ITEM_W *items = malloc(size);
        if (items && PdhGetFormattedCounterArrayW(percpu_intr, PDH_FMT_DOUBLE, &size, &count,
                                                  items) == ERROR_SUCCESS) {
            printf("  %-34s", "    Interrupts/sec by CPU:");
            for (DWORD i = 0; i < count; i++)
                if (wcscmp(items[i].szName, L"_Total") != 0)
                    printf(" %ls:%.0f", items[i].szName, items[i].FmtValue.doubleValue);
            printf("\n");
        }
        free(items);
    }
}

int main(int argc, char **argv)
{
    int seconds = argc > 1 ? atoi(argv[1]) : 3;
    if (seconds < 1)
        seconds = 1;
    setvbuf(stdout, NULL, _IONBF, 0);

    PDH_STATUS s = PdhOpenQueryW(NULL, 0, &q);
    if (s != ERROR_SUCCESS) {
        printf("PdhOpenQuery failed: 0x%08lx\n", (unsigned long)s);
        return 1;
    }
    for (int i = 0; i < NCOUNTERS; i++) {
        wchar_t path[128];
        swprintf(path, 128, L"\\Processor(_Total)\\%ls", names[i]);
        s = PdhAddEnglishCounterW(q, path, 0, &total[i]);
        if (s != ERROR_SUCCESS) {
            printf("PdhAddEnglishCounter(%ls) failed: 0x%08lx\n", path, (unsigned long)s);
            return 1;
        }
    }
    s = PdhAddEnglishCounterW(q, L"\\Processor(*)\\Interrupts/sec", 0, &percpu_intr);
    if (s != ERROR_SUCCESS)
        printf("per-CPU counter not available: 0x%08lx\n", (unsigned long)s);

    printf("Processor(_Total) counters, %d s per phase\n", seconds);
    printf("  %-34s", "phase");
    for (int i = 0; i < NCOUNTERS; i++)
        printf(" %12ls", names[i]);
    printf("\n");

    phase("a) quiet", seconds);
    HANDLE th = CreateThread(NULL, 0, timer_thread, NULL, 0, NULL);
    phase("b) Sleep(1) loop at 1 ms resolution", seconds);
    stop = 1;
    WaitForSingleObject(th, INFINITE);
    CloseHandle(th);
    printf("  %-34s %ld Sleep(1) calls returned in phase b\n", "", sleeps);
    phase("c) quiet again", seconds);
    PdhCloseQuery(q);
    return 0;
}

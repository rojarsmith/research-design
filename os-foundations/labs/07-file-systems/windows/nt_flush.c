// nt_flush.c - Lab 07, exercise J: who decides when dirty mapped pages
// reach the disk on NT?
//
//   1. a store through a view, and ReadFile through another handle
//   2. dirty every page of a view, then FlushViewOfFile, then
//      FlushFileBuffers, each timed
//   3. dirty every page, keep the view mapped for a while, then time
//      FlushViewOfFile again: were the pages still dirty?
//   4. dirty every page, unmap and close without flushing, and watch the
//      system's modified page list drain
//   5. the same amount of data through WriteFile (the cache manager):
//      FlushFileBuffers at once, and after a wait, each timed
// The counters come from PDH and count the whole machine, so other work
// shows up in them. A flush that takes as long as the first one found
// the pages still dirty; one that returns at once found nothing to do.
// No administrator rights are needed.
//
// usage: nt_flush.exe [directory] [MiB] [seconds to watch]
//        defaults: the current directory, 256 MiB, 30 s
// build: gcc -O2 -Wall -o nt_flush.exe nt_flush.c -lpdh

#include <windows.h>
#include <pdh.h>
#include <stdio.h>
#include <stdlib.h>

static PDH_HQUERY q;
static PDH_HCOUNTER c_modified, c_cachedirty;
static LARGE_INTEGER freq;

static void
die(const char *what)
{
  printf("%s failed, error %lu\n", what, GetLastError());
  ExitProcess(1);
}

static double
now(void)
{
  LARGE_INTEGER t;
  QueryPerformanceCounter(&t);
  return (double)t.QuadPart / freq.QuadPart;
}

static double
counter(PDH_HCOUNTER c)
{
  PDH_FMT_COUNTERVALUE v;
  if (PdhGetFormattedCounterValue(c, PDH_FMT_DOUBLE, NULL, &v) != ERROR_SUCCESS)
    return -1;
  return v.doubleValue;
}

// Modified Page List Bytes and Cache\Dirty Pages, both in MiB.
static void
state(const char *what)
{
  PdhCollectQueryData(q);
  printf("  %-46s modified page list %8.1f MiB | cache dirty %7.1f MiB\n",
         what, counter(c_modified) / 1048576.0,
         counter(c_cachedirty) * 4096.0 / 1048576.0);
}

static void
dirty_all(char *v, SIZE_T len, int round)
{
  for (SIZE_T i = 0; i < len; i += 4096)
    v[i] = (char)('a' + round);
}

static HANDLE
createfile(const char *path, DWORD disposition)
{
  HANDLE h = CreateFileA(path, GENERIC_READ | GENERIC_WRITE,
                         FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                         disposition, FILE_ATTRIBUTE_NORMAL, NULL);
  if (h == INVALID_HANDLE_VALUE)
    die("CreateFile");
  return h;
}

static char *
mapall(HANDLE f, SIZE_T len, HANDLE *m)
{
  *m = CreateFileMappingA(f, NULL, PAGE_READWRITE, 0, 0, NULL);
  if (!*m)
    die("CreateFileMapping");
  char *v = MapViewOfFile(*m, FILE_MAP_WRITE, 0, 0, len);
  if (!v)
    die("MapViewOfFile");
  return v;
}

// Watch the counters once a second for secs seconds.
static void
watch(int secs)
{
  for (int s = 1; s <= secs; s++) {
    Sleep(1000);
    if (s <= 5 || s % 5 == 0) {
      char what[32];
      snprintf(what, sizeof what, "%2d s later:", s);
      state(what);
    }
  }
}

int
main(int argc, char *argv[])
{
  const char *dir = argc > 1 ? argv[1] : ".";
  SIZE_T mib = argc > 2 ? strtoul(argv[2], NULL, 10) : 256;
  int secs = argc > 3 ? atoi(argv[3]) : 30;
  SIZE_T len = mib << 20;
  char path[MAX_PATH], path2[MAX_PATH], fsname[32] = "?";
  double t;
  HANDLE f, m;
  DWORD n;

  QueryPerformanceFrequency(&freq);
  if (PdhOpenQueryA(NULL, 0, &q) != ERROR_SUCCESS ||
      PdhAddEnglishCounterA(q, "\\Memory\\Modified Page List Bytes", 0,
                            &c_modified) != ERROR_SUCCESS ||
      PdhAddEnglishCounterA(q, "\\Cache\\Dirty Pages", 0, &c_cachedirty) !=
        ERROR_SUCCESS)
    die("PDH");

  snprintf(path, sizeof path, "%s\\flush.dat", dir);
  snprintf(path2, sizeof path2, "%s\\flush2.dat", dir);
  f = createfile(path, CREATE_ALWAYS);
  LARGE_INTEGER sz;
  sz.QuadPart = len;
  if (!SetFilePointerEx(f, sz, NULL, FILE_BEGIN) || !SetEndOfFile(f))
    die("SetEndOfFile");
  {
    WCHAR w[32];
    if (GetVolumeInformationByHandleW(f, NULL, 0, NULL, NULL, NULL, w, 32))
      WideCharToMultiByte(CP_UTF8, 0, w, -1, fsname, sizeof fsname, NULL, NULL);
  }
  printf("file %s on %s, %zu MiB\n", path, fsname, mib);
  char *v = mapall(f, len, &m);

  printf("1. a store through the view, and ReadFile through another handle\n");
  HANDLE f2 = createfile(path, OPEN_EXISTING);
  char ch = 0;
  OVERLAPPED o = {0};
  v[5] = 'M';
  o.Offset = 5;
  ReadFile(f2, &ch, 1, &n, &o);
  printf("  store 'M' at byte 5 of the view; ReadFile of byte 5 gives '%c'\n",
         ch);
  o.Offset = 6;
  WriteFile(f2, "W", 1, &n, &o);
  printf("  WriteFile 'W' at byte 6; the view shows '%c'\n", v[6]);
  CloseHandle(f2);

  printf("2. dirty every page of the view, then flush\n");
  FlushViewOfFile(v, 0);
  FlushFileBuffers(f);
  state("clean, after a first flush:");
  t = now();
  dirty_all(v, len, 1);
  printf("  stores to %zu pages took %.1f ms\n", len / 4096, (now() - t) * 1e3);
  state("after the stores:");
  t = now();
  if (!FlushViewOfFile(v, 0))
    die("FlushViewOfFile");
  printf("  FlushViewOfFile returned after %.1f ms\n", (now() - t) * 1e3);
  state("after FlushViewOfFile:");
  t = now();
  FlushViewOfFile(v, 0);
  printf("  FlushViewOfFile again (nothing dirty) took %.3f ms\n",
         (now() - t) * 1e3);
  t = now();
  if (!FlushFileBuffers(f))
    die("FlushFileBuffers");
  printf("  FlushFileBuffers returned after %.1f ms\n", (now() - t) * 1e3);
  state("after FlushFileBuffers:");

  printf("3. dirty every page and keep the view mapped for %d s\n", secs);
  dirty_all(v, len, 2);
  state("after the stores:");
  watch(secs);
  t = now();
  FlushViewOfFile(v, 0);
  printf("  FlushViewOfFile after the wait took %.1f ms\n", (now() - t) * 1e3);
  FlushFileBuffers(f);

  printf("4. dirty every page, unmap and close without flushing\n");
  dirty_all(v, len, 3);
  state("after the stores:");
  UnmapViewOfFile(v);
  CloseHandle(m);
  CloseHandle(f);
  state("right after UnmapViewOfFile and CloseHandle:");
  watch(secs);
  f = createfile(path, OPEN_EXISTING);
  v = mapall(f, len, &m);
  printf("  mapped again: byte 0 is '%c', byte %zu is '%c'\n", v[0], len - 4096,
         v[len - 4096]);
  UnmapViewOfFile(v);
  CloseHandle(m);
  CloseHandle(f);
  DeleteFileA(path);

  printf("5. the same amount through WriteFile, the cache manager's path\n");
  char *buf = malloc(1 << 20);
  memset(buf, 'w', 1 << 20);
  for (int round = 0; round < 2; round++) {
    f = createfile(path2, CREATE_ALWAYS);
    t = now();
    for (SIZE_T i = 0; i < mib; i++)
      if (!WriteFile(f, buf, 1 << 20, &n, NULL))
        die("WriteFile");
    printf("  WriteFile of %zu MiB took %.1f ms\n", mib, (now() - t) * 1e3);
    state("after the writes:");
    if (round == 1)
      watch(secs);
    t = now();
    if (!FlushFileBuffers(f))
      die("FlushFileBuffers");
    printf("  FlushFileBuffers %s took %.1f ms\n",
           round == 0 ? "at once" : "after the wait", (now() - t) * 1e3);
    CloseHandle(f);
    DeleteFileA(path2);
  }
  free(buf);
  return 0;
}

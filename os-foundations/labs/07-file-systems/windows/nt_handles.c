// nt_handles.c - Lab 07, exercise I: handles and file objects on NT, the
// counterpart of file descriptors and open file descriptions.
//
// CreateFile creates a new file object, which holds the current file
// position. DuplicateHandle, handle inheritance and DuplicateHandle into
// another process give a second handle to the same file object. Learn's
// DuplicateHandle page: "if you duplicate a file handle, the current file
// position is always the same for both handles". The access rights,
// though, belong to each handle.
//
// usage: nt_handles.exe [directory]      (default: the current directory)
// build: gcc -O2 -Wall -o nt_handles.exe nt_handles.c

#include <windows.h>
#include <stdio.h>
#include <string.h>

static char path[MAX_PATH];

static void
die(const char *what)
{
  printf("%s failed, error %lu\n", what, GetLastError());
  ExitProcess(1);
}

static void
put(HANDLE h, const char *s)
{
  DWORD n;
  if (!WriteFile(h, s, (DWORD)strlen(s), &n, NULL) || n != strlen(s))
    die("WriteFile");
}

static void
show(const char *what)
{
  char buf[64];
  DWORD n = 0;
  HANDLE h = CreateFileA(path, GENERIC_READ,
                         FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                         OPEN_EXISTING, 0, NULL);
  ReadFile(h, buf, sizeof buf - 1, &n, NULL);
  CloseHandle(h);
  buf[n] = 0;
  printf("  %-56s file: \"%s\"\n", what, buf);
}

static LONGLONG
pos(HANDLE h)
{
  LARGE_INTEGER zero = {0}, p;
  if (!SetFilePointerEx(h, zero, &p, FILE_CURRENT))
    die("SetFilePointerEx");
  return p.QuadPart;
}

static HANDLE
openrw(DWORD disposition)
{
  HANDLE h = CreateFileA(path, GENERIC_READ | GENERIC_WRITE,
                         FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                         disposition, FILE_ATTRIBUTE_NORMAL, NULL);
  if (h == INVALID_HANDLE_VALUE)
    die("CreateFile");
  return h;
}

// CompareObjectHandles is in KernelBase.dll (Windows 10 and later).
typedef BOOL(WINAPI *cmp_t)(HANDLE, HANDLE);

static void
compare(HANDLE a, HANDLE b)
{
  static cmp_t cmp;
  if (!cmp)
    cmp = (cmp_t)(void (*)(void))GetProcAddress(
      GetModuleHandleA("kernelbase.dll"), "CompareObjectHandles");
  BY_HANDLE_FILE_INFORMATION ia, ib;
  GetFileInformationByHandle(a, &ia);
  GetFileInformationByHandle(b, &ib);
  printf("    handles %p and %p: CompareObjectHandles %s; file index "
         "%08lx%08lx and %08lx%08lx\n",
         a, b,
         cmp ? (cmp(a, b) ? "TRUE (one file object)"
                          : "FALSE (two file objects)")
             : "(not available)",
         ia.nFileIndexHigh, ia.nFileIndexLow, ib.nFileIndexHigh,
         ib.nFileIndexLow);
}

// The child: write through a handle it was given, then exit.
static int
child(const char *how, const char *arg)
{
  HANDLE h;
  if (strcmp(how, "inherit") == 0) {
    h = (HANDLE)(ULONG_PTR)_strtoui64(arg, NULL, 16);
    put(h, "child.");
  } else { // "dup": read the handle value from standard input
    char v[32] = "";
    DWORD n = 0;
    ReadFile(GetStdHandle(STD_INPUT_HANDLE), v, sizeof v - 1, &n, NULL);
    v[n] = 0;
    h = (HANDLE)(ULONG_PTR)_strtoui64(v, NULL, 16);
    put(h, "dup'd.");
  }
  return 0;
}

static void
spawn(const char *args, HANDLE in, PROCESS_INFORMATION *pi, BOOL suspended)
{
  char exe[MAX_PATH], cmd[2 * MAX_PATH];
  STARTUPINFOA si = {sizeof si};
  GetModuleFileNameA(NULL, exe, sizeof exe);
  snprintf(cmd, sizeof cmd, "\"%s\" %s", exe, args);
  if (in) {
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = in;
    si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
  }
  if (!CreateProcessA(exe, cmd, NULL, NULL, TRUE,
                      suspended ? CREATE_SUSPENDED : 0, NULL, NULL, &si, pi))
    die("CreateProcess");
}

int
main(int argc, char *argv[])
{
  if (argc > 2 && strcmp(argv[1], "child") == 0)
    return child(argv[2], argc > 3 ? argv[3] : "");

  const char *dir = argc > 1 ? argv[1] : ".";
  snprintf(path, sizeof path, "%s\\handles.txt", dir);
  HANDLE a, b, c, d;
  DWORD n;

  printf("1. DuplicateHandle: two handles, one file object\n");
  a = openrw(CREATE_ALWAYS);
  if (!DuplicateHandle(GetCurrentProcess(), a, GetCurrentProcess(), &b, 0,
                       FALSE, DUPLICATE_SAME_ACCESS))
    die("DuplicateHandle");
  put(a, "AAAA");
  put(b, "BBBB");
  show("write AAAA via handle a, then BBBB via its duplicate b:");
  LARGE_INTEGER zero = {0};
  SetFilePointerEx(a, zero, NULL, FILE_BEGIN);
  printf("    after SetFilePointerEx(a, 0, FILE_BEGIN): position via a %lld, "
         "via b %lld\n",
         pos(a), pos(b));
  compare(a, b);
  CloseHandle(a);
  CloseHandle(b);

  printf("2. CreateFile twice: two file objects, one file\n");
  c = openrw(OPEN_EXISTING);
  d = openrw(OPEN_EXISTING);
  put(c, "cccc");
  put(d, "dd");
  show("write cccc via handle c, then dd via handle d:");
  printf("    position via c %lld, via d %lld\n", pos(c), pos(d));
  compare(c, d);
  CloseHandle(c);
  CloseHandle(d);

  printf("3. a child process gets the parent's file object\n");
  a = openrw(TRUNCATE_EXISTING);
  SetHandleInformation(a, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
  char args[64];
  snprintf(args, sizeof args, "child inherit %llx",
           (unsigned long long)(ULONG_PTR)a);
  PROCESS_INFORMATION pi;
  spawn(args, NULL, &pi, FALSE);
  WaitForSingleObject(pi.hProcess, INFINITE);
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  put(a, "parent");
  show("inherited: the child writes child., then the parent writes:");
  SetHandleInformation(a, HANDLE_FLAG_INHERIT, 0);

  // DuplicateHandle into another process: the child learns the value of
  // its new handle through a pipe (stage 08 uses a named pipe for this).
  HANDLE rd, wr;
  SECURITY_ATTRIBUTES sa = {sizeof sa, NULL, TRUE};
  if (!CreatePipe(&rd, &wr, &sa, 0))
    die("CreatePipe");
  SetHandleInformation(wr, HANDLE_FLAG_INHERIT, 0);
  spawn("child dup", rd, &pi, TRUE);
  HANDLE remote;
  if (!DuplicateHandle(GetCurrentProcess(), a, pi.hProcess, &remote, 0, FALSE,
                       DUPLICATE_SAME_ACCESS))
    die("DuplicateHandle into the child");
  snprintf(args, sizeof args, "%llx", (unsigned long long)(ULONG_PTR)remote);
  WriteFile(wr, args, (DWORD)strlen(args), &n, NULL);
  CloseHandle(wr);
  ResumeThread(pi.hThread);
  WaitForSingleObject(pi.hProcess, INFINITE);
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  CloseHandle(rd);
  put(a, "PARENT");
  show("DuplicateHandle into a child: it writes dup'd., then the parent:");
  printf("    the parent's position is now %lld\n", pos(a));

  printf("4. access rights belong to the handle, the position to the file "
         "object\n");
  if (!DuplicateHandle(GetCurrentProcess(), a, GetCurrentProcess(), &b,
                       FILE_GENERIC_READ, FALSE, 0))
    die("DuplicateHandle (read only)");
  SetFilePointerEx(a, zero, NULL, FILE_BEGIN);
  BOOL w = WriteFile(b, "x", 1, &n, NULL);
  DWORD err = GetLastError();
  printf("    WriteFile via the read-only duplicate: %s", w ? "TRUE" : "FALSE");
  if (!w)
    printf(", error %lu%s", err,
           err == ERROR_ACCESS_DENIED ? " (ERROR_ACCESS_DENIED)" : "");
  printf("\n");
  char buf[8];
  ReadFile(b, buf, 6, &n, NULL);
  printf("    ReadFile of 6 bytes via the duplicate; position via a is now "
         "%lld\n",
         pos(a));
  CloseHandle(b);
  CloseHandle(a);

  DeleteFileA(path);
  return 0;
}

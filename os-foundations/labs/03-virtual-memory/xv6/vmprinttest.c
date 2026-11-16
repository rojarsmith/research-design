// vmprinttest.c - Stage 03, exercises A and F: print this process's page
// table with the vmprint() system call, and leave a marker string in a
// page of its own for the hand translation in exercise F.
//
// Needs the vmprint() system call you add in exercise A:
//   int vmprint(void);   prints the calling process's page table
//
// Install: copy to user/vmprinttest.c and add $U/_vmprinttest to UPROGS.
// Run: vmprinttest        (this process's page table)
//      vmprinttest fork   (a forked child's page table instead)

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// Initialized, page-aligned and one page long, so it sits alone in one
// data page.
char marker[4096] __attribute__((aligned(4096))) =
  "lab03 marker: translate my address by hand";

int
main(int argc, char *argv[])
{
  int local = 0;
  char *eager, *lazy;

  eager = sbrk(4096);        // one page, mapped now
  eager[0] = 'e';
  lazy = sbrklazy(3 * 4096); // three pages, mapped on first touch
  lazy[4096] = 'l';          // touch only the second one

  printf("text   main    %p\n", (void *)main);
  printf("data   marker  %p\n", (void *)marker);
  printf("stack  local   %p\n", (void *)&local);
  printf("heap   eager   %p  (1 page from sbrk)\n", (void *)eager);
  printf("heap   lazy    %p  (3 pages from sbrklazy, only the 2nd touched)\n",
         (void *)lazy);
  printf("size   sbrk(0) %p\n", (void *)sbrk(0));
  if (argc > 1 && strcmp(argv[1], "fork") == 0) {
    int pid = fork();
    if (pid == 0) {
      printf("the child's page table, just after fork:\n");
      vmprint();
      exit(0);
    }
    wait(0);
    exit(0);
  }
  vmprint();
  exit(0);
}

// volatile must survive -O2; C11 atomics need hardware support or a libcall.
#include <stdatomic.h>
volatile int *const REG = (volatile int *)0x1000;
int poll(void) { int n = 0; while (*REG == 0) n++; return n; }
void set_reg(int v) { *REG = v; *REG = v; }   // both stores must stay
atomic_int counter;
int bump(void) { return atomic_fetch_add(&counter, 1); }
int load_acq(void) { return atomic_load_explicit(&counter, memory_order_acquire); }

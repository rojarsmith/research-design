/* Host-side implementation of the tinyc builtins. On the simulator these are
 * environment calls; on a hosted system they are ordinary library calls. */
#include <stdio.h>
int print_int(int v) { printf("%d", v); return 0; }
int print_char(int c) { putchar(c); return 0; }

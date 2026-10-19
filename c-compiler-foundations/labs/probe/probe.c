/* probe.c: ask a C implementation (compiler + ABI + target) what it decided
 * about the things the C standard leaves to it. Build it with any compiler
 * for any target; the answers are the "implementation-defined" contract you
 * must know before trusting code on that processor.
 *
 *   cc -std=c11 probe.c -o probe && ./probe
 *   riscv64-unknown-elf-gcc -march=rv32im -mabi=ilp32 probe.c ...   (cross)
 */
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>

struct align_probe { char c; double d; };

int main(void) {
    unsigned int one = 1;
    unsigned char *first = (unsigned char *)&one;
    int neg = -1;

    printf("__STDC_VERSION__      %ld\n", (long)__STDC_VERSION__);
    printf("__STDC_HOSTED__       %d\n", __STDC_HOSTED__);
    printf("CHAR_BIT              %d\n", CHAR_BIT);
    printf("char is signed        %s\n", (CHAR_MIN < 0) ? "yes" : "no");
    printf("sizeof short/int/long %zu/%zu/%zu\n", sizeof(short), sizeof(int), sizeof(long));
    printf("sizeof long long      %zu\n", sizeof(long long));
    printf("sizeof pointer        %zu\n", sizeof(void *));
    printf("sizeof size_t         %zu\n", sizeof(size_t));
    printf("sizeof float/double   %zu/%zu\n", sizeof(float), sizeof(double));
    printf("alignof double        %zu\n", offsetof(struct align_probe, d));
    printf("endianness            %s\n", (*first == 1) ? "little" : "big");
    printf("INT_MAX               %d\n", INT_MAX);
    printf("-1 >> 1               %d  (%s shift)\n", neg >> 1, (neg >> 1) == -1 ? "arithmetic" : "logical");
    printf("-7 / 2, -7 %% 2        %d, %d\n", -7 / 2, -7 % 2);
    printf("(char)300             %d\n", (int)(char)300);
    printf("data model            %s\n",
           sizeof(long) == 8 && sizeof(void *) == 8 ? "LP64" :
           sizeof(long) == 4 && sizeof(void *) == 8 ? "LLP64" :
           sizeof(int) == 4 && sizeof(void *) == 4 ? "ILP32" : "other");
#if defined(__riscv)
    printf("arch                  riscv xlen=%d\n", __riscv_xlen);
#elif defined(__x86_64__)
    printf("arch                  x86_64\n");
#elif defined(__i386__)
    printf("arch                  i386\n");
#elif defined(__aarch64__)
    printf("arch                  aarch64\n");
#elif defined(__arm__)
    printf("arch                  arm32\n");
#else
    printf("arch                  (unknown to this probe)\n");
#endif
    return 0;
}

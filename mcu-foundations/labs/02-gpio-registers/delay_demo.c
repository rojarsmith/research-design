/*
 * delay_demo.c：同一段「延遲迴圈」，換個編譯選項就變成完全不同的時間。
 *
 * 分別用 -O0 與 -O2 建置，比較兩次輸出：
 *   gcc -std=c11 -Wall -Wextra -Werror -O0 -o delay_O0 delay_demo.c
 *   gcc -std=c11 -Wall -Wextra -Werror -O2 -o delay_O2 delay_demo.c
 *
 * 沒有 volatile 的空迴圈沒有可觀察的效果，最佳化時可以整個移除；
 * 加了 volatile 的版本每次都要真的讀寫記憶體，但花多久仍取決於 CPU、
 * 時脈、記憶體等待週期與編譯器。在 MCU 上同樣成立，所以延遲迴圈不是時間來源。
 */
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#define ITERATIONS 200000000u

static double now_ms(void)
{
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1e6;
}

static void delay_plain(uint32_t n)
{
    for (uint32_t i = 0; i < n; i++) {
    }
}

static void delay_volatile(uint32_t n)
{
    for (volatile uint32_t i = 0; i < n; i++) {
    }
}

int main(void)
{
    double t0 = now_ms();
    delay_plain(ITERATIONS);
    double t1 = now_ms();
    delay_volatile(ITERATIONS);
    double t2 = now_ms();

    double plain = t1 - t0, vol = t2 - t1;
#ifdef __OPTIMIZE__
    printf("optimization: on\n");
#else
    printf("optimization: off\n");
#endif
    printf("plain loop    %u iterations: %8.1f ms\n", ITERATIONS, plain);
    printf("volatile loop %u iterations: %8.1f ms\n", ITERATIONS, vol);
    printf("if someone assumed 1 iteration = 1 cycle, the volatile loop would imply %.0f MHz\n",
           vol > 0.0 ? ITERATIONS / (vol * 1000.0) : 0.0);
    if (plain < vol / 100.0)
        printf("observation: the plain loop was removed or nearly removed by the compiler\n");
    else
        printf("observation: the plain loop still runs; compare with an optimized build\n");
    return 0;
}

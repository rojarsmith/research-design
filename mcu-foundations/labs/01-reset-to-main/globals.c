/*
 * globals.c：在桌機上練習讀 map file 與符號表。
 *
 * 先在每個變數旁寫下你的預測：它會在 .data、.bss，還是唯讀資料區？
 * Windows（MinGW）的唯讀資料區叫 .rdata，Linux ELF 叫 .rodata。
 *
 * 建置：gcc -std=c11 -Wall -Wextra -Werror -O0 -Wl,-Map=globals.map -o globals globals.c
 * 觀察：nm globals.exe（Linux 為 nm globals）、size globals.exe、打開 globals.map
 */
#include <stdint.h>
#include <stdio.h>

uint32_t g_sample_period_ms = 10;     /* 預測： */
uint32_t g_sample_count;              /* 預測： */
uint32_t g_zero_init = 0;             /* 預測：初值寫了 0，算 .data 還是 .bss？ */
static uint8_t s_rx_buffer[256];      /* 預測：map 的符號清單裡找得到嗎？ */
const char k_fw_name[] = "logger-v1"; /* 預測： */

int main(void)
{
    static uint32_t s_calls; /* 函式內的 static：儲存期和全域變數相同 */
    s_calls++;
    s_rx_buffer[0] = (uint8_t)s_calls;

    /* 桌機作業系統載入程式時可能重新安排基底地址，所以這裡印出的地址
     * 可能和 nm、map 裡的值差一個固定偏移；MCU 沒有載入器，map 就是執行地址。 */
    printf("g_sample_period_ms %p\n", (void *)&g_sample_period_ms);
    printf("g_sample_count     %p\n", (void *)&g_sample_count);
    printf("g_zero_init        %p\n", (void *)&g_zero_init);
    printf("s_rx_buffer        %p\n", (void *)s_rx_buffer);
    printf("k_fw_name          %p\n", (void *)k_fw_name);
    printf("s_calls            %p\n", (void *)&s_calls);
    printf("values: period=%u count=%u zero=%u name=%s\n", (unsigned)g_sample_period_ms,
           (unsigned)g_sample_count, (unsigned)g_zero_init, k_fw_name);
    return 0;
}

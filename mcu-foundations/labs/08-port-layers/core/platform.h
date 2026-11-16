/*
 * platform.h：應用核心和平台之間唯一的介面。
 *
 * 應用核心（core/app.c 加上第五課原封不動的 logger.c、frame.c、ring.c）只認得這個檔案。
 * 兩個方向各四個入口，型別只用 <stdint.h> 與 <stdbool.h>：
 * 沒有廠商的 handle、狀態碼、callback 型別，也沒有中斷向量名稱。
 *
 * 平台要提供給應用的四件事（platform_ops_t）：
 *   now_ms        單調遞增的毫秒計數，32 位元、允許回繞。硬體計數器是 16 位元、
 *                 頻率不是 1 kHz，都由平台換算，應用不知道。只在主迴圈呼叫。
 *   tx_kick       TX 緩衝區有資料了。平台決定怎麼開始送：開 TX 空中斷、啟動 HAL 的
 *                 非阻塞傳送或 DMA。只在主迴圈呼叫，可以呼叫很多次。
 *   sensor_start  開始一次量測，不等待。
 *   sensor_poll   完成時回傳 1 並寫入 value，還沒好回傳 0，不等待。
 *                 逾時由應用判斷；平台的 callback 只能設旗標，不能直接呼叫應用的邏輯。
 *
 * 平台呼叫應用的四個入口：
 *   app_init      開機時呼叫一次，傳入上面的四個函式。
 *   app_poll      主迴圈每一圈呼叫一次。
 *   app_uart_rx   收到一個 byte。可以在中斷或 HAL 的 callback 裡呼叫（它只放進 SPSC 緩衝區）。
 *   app_uart_tx   取出最多 max 個要送出的 byte。可以在中斷、callback 或主迴圈呼叫，
 *                 但同一時間只能有一個地方在取，由平台保證。
 */
#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint32_t (*now_ms)(void);
    void (*tx_kick)(void);
    void (*sensor_start)(void);
    int (*sensor_poll)(int16_t *value);
} platform_ops_t;

void app_init(const platform_ops_t *ops);
void app_poll(void);
bool app_uart_rx(uint8_t byte);
uint32_t app_uart_tx(uint8_t *dst, uint32_t max);

#endif

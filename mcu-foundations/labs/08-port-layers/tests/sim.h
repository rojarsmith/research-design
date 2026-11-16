/*
 * sim.h：協定測試看到的「外界」。每個假平台的 sim_x.c 各自實作一份。
 *
 * 協定測試只透過這些函式觀察裝置：送 byte 進 UART、推進時間、拔插感測器、
 * 收下 UART 送出的 byte。它只用第五課的 frame.h 與 host_rx.h（電腦端也用同一份協定），
 * 不 include 任何平台標頭，也不讀裝置的內部變數；裝置的計數一律用 GET_STATUS 問。
 * 所以同一份 protocol_tests.c 可以不改一個字，分別和平台 A、平台 B 連結。
 */
#ifndef SIM_H
#define SIM_H

#include <stdint.h>

/* 電腦收到裝置送出的 byte 時呼叫；now_ms 是真實時間 */
typedef void (*host_sink_t)(void *ctx, const uint8_t *data, uint32_t n, uint32_t now_ms);

const char *sim_platform(void);
/* 重新上電：清空硬體模型，呼叫 board_init(time_origin_ms) */
void sim_reset(uint32_t time_origin_ms, host_sink_t sink, void *ctx);
void sim_run_ms(uint32_t ms);
uint32_t sim_now_ms(void);
void sim_host_send(const uint8_t *data, uint32_t n); /* 依 115,200 baud 的速率送進裝置 */
void sim_block_main(uint32_t ms);                    /* 主迴圈接下來 ms 毫秒不執行，中斷照常 */
void sim_sensor_connect(int connected);
void sim_print_stats(void);

/* 每塊板子的 board_x.c 提供：相當於真實韌體的 main() 拆成兩段。
 *   int main(void) { board_init(0); for (;;) board_main_once(); }
 * time_origin_ms 讓測試能從接近回繞的地方開始，上板時傳 0。 */
void board_init(uint32_t time_origin_ms);
void board_main_once(void);

#endif

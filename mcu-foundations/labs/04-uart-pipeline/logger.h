/*
 * logger.h：資料採集器的應用核心，與硬體無關。
 *
 * 邊界：
 *   硬體相關的部分只透過三個入口進出——
 *     logger_uart_rx_isr()  由 UART 接收中斷呼叫，把一個位元組放進 RX 環形緩衝區
 *     logger_uart_tx_isr()  由 UART 傳送中斷呼叫，取出下一個要送的位元組
 *     sensor_port_t         由板級程式實作的感測器存取（I2C、SPI、ADC 都可以）
 *   其餘全部在主迴圈的 logger_poll() 裡執行，每次只做有限的工作就返回。
 *
 * 緩衝區策略：
 *   TX 緩衝區滿時，丟棄「整個」新的取樣封包並計數；序號照樣遞增，
 *   電腦端看到序號跳號就知道少了幾筆，下一個送出的封包也會帶 STATUS_OVERFLOW。
 *   取樣封包只在剩餘空間至少有「封包 + 保留量」時才放入，保留量留給命令回應，
 *   所以超載時電腦仍收得到 ACK。
 */
#ifndef LOGGER_H
#define LOGGER_H

#include <stdbool.h>
#include <stdint.h>

#include "frame.h"
#include "ring.h"

#define LOGGER_TX_CAPACITY 1024u
#define LOGGER_RX_CAPACITY 64u
#define LOGGER_TX_RESERVE 16u          /* 保留給 ACK/STATUS 的空間 */
#define LOGGER_RX_BUDGET 16u           /* 每次 poll 最多處理幾個接收位元組 */
#define LOGGER_SENSOR_TIMEOUT_MS 2u
#define LOGGER_PARSER_TIMEOUT_MS 20u
#define LOGGER_PERIOD_MIN_MS 1u
#define LOGGER_PERIOD_MAX_MS 1000u

/* 板級程式實作：start 開始一次量測；poll 回傳 1 表示完成並寫入 value，0 表示還沒好 */
typedef struct {
    void (*start)(void *ctx);
    int (*poll)(void *ctx, int16_t *value);
    void *ctx;
} sensor_port_t;

typedef struct {
    ring_t tx, rx;
    uint8_t tx_mem[LOGGER_TX_CAPACITY];
    uint8_t rx_mem[LOGGER_RX_CAPACITY];
    parser_t parser;
    sensor_port_t sensor;

    uint16_t period_ms;
    uint32_t last_slot_ms;   /* 上一個取樣期限（毫秒計數） */
    int sensor_busy;
    uint32_t sensor_start_ms;
    uint32_t busy_slot_ms;   /* 進行中的量測屬於哪一個取樣期限 */
    uint16_t seq;
    uint8_t pending_flags;   /* 要帶在下一個送出的取樣封包上的 status */
    uint32_t now_ms;

    /* 可觀察的計數；全部由主迴圈寫入 */
    uint32_t samples_queued, dropped_frames, missed_periods, sensor_errors;
    uint32_t responses_dropped;
} logger_t;

void logger_init(logger_t *lg, sensor_port_t sensor, uint32_t now_ms, uint32_t tx_start_index);
void logger_poll(logger_t *lg, uint32_t now_ms);

/* ISR 端入口 */
bool logger_uart_rx_isr(logger_t *lg, uint8_t byte);
bool logger_uart_tx_isr(logger_t *lg, uint8_t *byte);

uint32_t logger_rx_overflow(logger_t *lg);

#endif

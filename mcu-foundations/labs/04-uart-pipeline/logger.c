/*
 * logger.c：資料採集器的應用核心。不包含任何暫存器存取或廠商型別。
 */
#include "logger.h"

#include <string.h>

static uint16_t sat16(uint32_t v) { return v > 0xFFFFu ? 0xFFFFu : (uint16_t)v; }

/* 命令回應：可以使用保留空間；連保留空間都不夠時才放棄並計數 */
static void send_response(logger_t *lg, uint8_t type, const uint8_t *payload, uint8_t len)
{
    uint8_t buf[FRAME_MAX_SIZE];
    uint32_t n = frame_encode(type, payload, len, buf);
    if (!ring_write_all(&lg->tx, buf, n)) lg->responses_dropped++;
}

static void ack(logger_t *lg, uint8_t cmd, uint8_t result)
{
    uint8_t p[2] = {cmd, result};
    send_response(lg, TYPE_ACK, p, sizeof p);
}

uint32_t logger_rx_overflow(logger_t *lg) { return ring_dropped(&lg->rx); }

static void on_command(void *ctx, const frame_t *f)
{
    logger_t *lg = ctx;
    switch (f->type) {
    case TYPE_SET_PERIOD: {
        if (f->len != 2u) {
            ack(lg, f->type, RESULT_BAD_LENGTH);
            return;
        }
        uint16_t v = get_u16(f->payload);
        if (v < LOGGER_PERIOD_MIN_MS || v > LOGGER_PERIOD_MAX_MS) {
            ack(lg, f->type, RESULT_OUT_OF_RANGE); /* 無效設定：保留原值，明確回報 */
            return;
        }
        lg->period_ms = v;
        lg->last_slot_ms = lg->now_ms; /* 決策：新週期從收到命令的時刻起算 */
        ack(lg, f->type, RESULT_OK);
        return;
    }
    case TYPE_GET_STATUS: {
        if (f->len != 0u) {
            ack(lg, f->type, RESULT_BAD_LENGTH);
            return;
        }
        uint8_t p[10];
        put_u16(&p[0], lg->period_ms);
        put_u16(&p[2], sat16(lg->dropped_frames));
        put_u16(&p[4], sat16(parser_errors(&lg->parser) + logger_rx_overflow(lg)));
        put_u16(&p[6], sat16(lg->sensor_errors));
        put_u16(&p[8], sat16(lg->missed_periods));
        send_response(lg, TYPE_STATUS, p, sizeof p);
        return;
    }
    default:
        ack(lg, f->type, RESULT_UNKNOWN_TYPE);
        return;
    }
}

void logger_init(logger_t *lg, sensor_port_t sensor, uint32_t now_ms, uint32_t tx_start_index)
{
    memset(lg, 0, sizeof *lg);
    ring_init(&lg->tx, lg->tx_mem, LOGGER_TX_CAPACITY, tx_start_index);
    ring_init(&lg->rx, lg->rx_mem, LOGGER_RX_CAPACITY, 0u);
    parser_init(&lg->parser, LOGGER_PARSER_TIMEOUT_MS, on_command, lg);
    lg->sensor = sensor;
    lg->period_ms = 10u;
    lg->last_slot_ms = now_ms;
    lg->now_ms = now_ms;
}

static void emit_sample(logger_t *lg, uint32_t slot_ms, int16_t value, uint8_t flags)
{
    uint8_t p[SAMPLE_PAYLOAD];
    put_u16(&p[0], lg->seq);
    put_u16(&p[2], (uint16_t)slot_ms);
    put_u16(&p[4], (uint16_t)value);
    p[6] = (uint8_t)(flags | lg->pending_flags);
    lg->seq++; /* 不論送不送得出去，序號都前進，電腦端才看得出遺失 */

    uint8_t buf[SAMPLE_FRAME_SIZE];
    uint32_t n = frame_encode(TYPE_SAMPLE, p, SAMPLE_PAYLOAD, buf);
    if (ring_free(&lg->tx) < n + LOGGER_TX_RESERVE) {
        lg->dropped_frames++;
        lg->pending_flags |= STATUS_OVERFLOW;
        return;
    }
    /* 只有主迴圈會寫 TX：看到的剩餘空間只會因 ISR 取走資料而變大，所以這裡一定成功 */
    ring_write_all(&lg->tx, buf, n);
    lg->samples_queued++;
    lg->pending_flags = 0;
}

void logger_poll(logger_t *lg, uint32_t now_ms)
{
    lg->now_ms = now_ms;

    /* 1. 接收：每次最多處理 LOGGER_RX_BUDGET 個位元組，避免一次大量輸入卡住取樣 */
    uint8_t b;
    for (uint32_t i = 0; i < LOGGER_RX_BUDGET && ring_pop(&lg->rx, &b); i++)
        parser_feed(&lg->parser, b, now_ms);

    /* 2. 取樣期限：保持相位，跳過的期數記下來（lessons/03 的 skip+count 策略） */
    uint32_t elapsed = now_ms - lg->last_slot_ms;
    if (elapsed >= lg->period_ms) {
        uint32_t periods = elapsed / lg->period_ms;
        lg->last_slot_ms += periods * lg->period_ms;
        lg->missed_periods += periods - 1u;
        if (lg->sensor_busy) {
            lg->missed_periods++; /* 上一次量測還沒結束，這一期放棄 */
        } else {
            lg->sensor.start(lg->sensor.ctx);
            lg->sensor_busy = 1;
            lg->sensor_start_ms = now_ms;
            lg->busy_slot_ms = lg->last_slot_ms;
        }
    }

    /* 3. 感測器：輪詢完成或逾時，絕不在這裡等待 */
    if (lg->sensor_busy) {
        int16_t v;
        if (lg->sensor.poll(lg->sensor.ctx, &v)) {
            lg->sensor_busy = 0;
            emit_sample(lg, lg->busy_slot_ms, v, 0u);
        } else if ((uint32_t)(now_ms - lg->sensor_start_ms) >= LOGGER_SENSOR_TIMEOUT_MS) {
            lg->sensor_busy = 0;
            lg->sensor_errors++;
            emit_sample(lg, lg->busy_slot_ms, 0, STATUS_SENSOR_TIMEOUT);
        }
    }
}

bool logger_uart_rx_isr(logger_t *lg, uint8_t byte) { return ring_push(&lg->rx, byte); }

bool logger_uart_tx_isr(logger_t *lg, uint8_t *byte) { return ring_pop(&lg->tx, byte); }

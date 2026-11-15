/*
 * host_rx.h：電腦端接收器。解析裝置送來的封包、檢查序號連續性、統計結果。
 * 與裝置端共用 frame.c，所以協定只有一份定義。
 */
#ifndef HOST_RX_H
#define HOST_RX_H

#include <stdint.h>

#include "frame.h"

typedef struct {
    parser_t parser;
    int have_seq;
    uint16_t last_seq;
    uint32_t samples, lost_samples, gap_events, overflow_flags, sensor_timeouts;
    uint32_t acks, nacks, statuses;
    uint8_t last_ack_cmd, last_ack_result;
    uint16_t status[5];            /* 最近一次 STATUS：period、dropped、rx_errors、sensor_errors、missed */
    uint16_t last_tick, min_tick_delta, max_tick_delta;
} host_rx_t;

void host_rx_init(host_rx_t *h);
void host_rx_feed(host_rx_t *h, const uint8_t *data, uint32_t n, uint32_t now_ms);

#endif

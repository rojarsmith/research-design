/*
 * host_rx.c：電腦端接收器實作。
 */
#include "host_rx.h"

#include <string.h>

static void on_frame(void *ctx, const frame_t *f)
{
    host_rx_t *h = ctx;
    if (f->type == TYPE_SAMPLE && f->len == SAMPLE_PAYLOAD) {
        uint16_t seq = get_u16(&f->payload[0]);
        uint16_t tick = get_u16(&f->payload[2]);
        uint8_t status = f->payload[6];
        if (h->have_seq) {
            /* 注意：uint16_t 相減會先被提升成 int，必須把結果轉回 uint16_t 才有模數語意 */
            uint16_t step = (uint16_t)(seq - h->last_seq);
            if (step != 1u) {
                h->gap_events++;
                h->lost_samples += (uint16_t)(step - 1u);
            }
            uint16_t dt = (uint16_t)(tick - h->last_tick);
            if (step == 1u) {
                if (dt < h->min_tick_delta) h->min_tick_delta = dt;
                if (dt > h->max_tick_delta) h->max_tick_delta = dt;
            }
        }
        h->have_seq = 1;
        h->last_seq = seq;
        h->last_tick = tick;
        h->samples++;
        if (status & STATUS_OVERFLOW) h->overflow_flags++;
        if (status & STATUS_SENSOR_TIMEOUT) h->sensor_timeouts++;
    } else if (f->type == TYPE_ACK && f->len == 2u) {
        h->last_ack_cmd = f->payload[0];
        h->last_ack_result = f->payload[1];
        if (f->payload[1] == RESULT_OK)
            h->acks++;
        else
            h->nacks++;
    } else if (f->type == TYPE_STATUS && f->len == 10u) {
        for (int i = 0; i < 5; i++) h->status[i] = get_u16(&f->payload[2 * i]);
        h->statuses++;
    }
}

void host_rx_init(host_rx_t *h)
{
    memset(h, 0, sizeof *h);
    h->min_tick_delta = 0xFFFFu;
    parser_init(&h->parser, 1000u, on_frame, h);
}

void host_rx_feed(host_rx_t *h, const uint8_t *data, uint32_t n, uint32_t now_ms)
{
    for (uint32_t i = 0; i < n; i++) parser_feed(&h->parser, data[i], now_ms);
}

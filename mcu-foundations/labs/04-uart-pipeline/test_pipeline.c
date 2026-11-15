/*
 * test_pipeline.c：資料採集器在桌機上的整合測試。
 *
 * 建置（一行）：
 *   gcc -std=c11 -Wall -Wextra -Werror -O2 -DRING_SIMULATE_PREEMPTION -o test_pipeline test_pipeline.c ring.c frame.c logger.c host_rx.c
 * 執行：
 *   ./test_pipeline
 *
 * 內容：
 *   1. 環形緩衝區基本行為與索引回繞
 *   2. ISR 與主迴圈交錯：在每個可被打斷的位置插入模擬 ISR
 *   3. 錯誤示範：兩邊共用一個 count 做讀-改-寫
 *   4. CRC 與封包解析：雜訊、半途連線、CRC 錯、過長、逾時
 *   5. 裝置 + UART + 電腦端整合：正常、命令、超載、感測器斷線、接收溢位、序號回繞
 */
#include <stdio.h>
#include <string.h>

#include "frame.h"
#include "host_rx.h"
#include "logger.h"
#include "ring.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h> /* 只用來讓主控台以 UTF-8 顯示中文 */
#endif

static void console_utf8(void)
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
}

static int g_fail;
#define CHECK(cond)                                                       \
    do {                                                                  \
        if (cond) {                                                       \
            printf("  ok   %s\n", #cond);                                 \
        } else {                                                          \
            printf("  FAIL %s  (line %d)\n", #cond, __LINE__);            \
            g_fail++;                                                     \
        }                                                                 \
    } while (0)

static uint32_t lcg_state = 1u;
static uint32_t lcg(void)
{
    lcg_state = lcg_state * 1664525u + 1013904223u;
    return lcg_state >> 8;
}

/* ================= 模擬中斷 ================= */
static void (*g_isr)(void);
static int g_in_isr;
static uint32_t g_isr_chance; /* 每個打斷點觸發 ISR 的機率，以 1/n 表示；0 表示關閉 */

/* ring.c 在每個可被打斷的位置呼叫這裡 */
void ring_sim_preempt(void)
{
    if (!g_isr || g_in_isr || g_isr_chance == 0u) return;
    if (lcg() % g_isr_chance != 0u) return;
    g_in_isr = 1; /* 單核心：ISR 執行期間主程式不會插進來 */
    g_isr();
    g_in_isr = 0;
}

/* ---- 1. 基本行為 ---- */
static void test_ring_basics(void)
{
    printf("環形緩衝區：容量必須是 2 的次方，索引從接近回繞處開始\n");
    ring_t r;
    uint8_t mem[8];
    CHECK(!ring_init(&r, mem, 6u, 0u));
    CHECK(ring_init(&r, mem, 8u, 0xFFFFFFFAu));
    int ok = 1;
    for (int i = 0; i < 8; i++) ok &= ring_push(&r, (uint8_t)i);
    CHECK(ok && ring_count(&r) == 8u && ring_free(&r) == 0u);
    CHECK(!ring_push(&r, 99u) && ring_dropped(&r) == 1u);
    uint8_t b = 0;
    ok = 1;
    for (int i = 0; i < 5; i++) ok &= ring_pop(&r, &b) && b == (uint8_t)i;
    CHECK(ok && ring_count(&r) == 3u);
    uint8_t six[6] = {10, 11, 12, 13, 14, 15};
    CHECK(!ring_write_all(&r, six, 6u) && ring_count(&r) == 3u); /* 只剩 5 格：整包拒絕 */
    CHECK(ring_write_all(&r, six, 5u) && ring_count(&r) == 8u);
    uint8_t out[8];
    uint32_t n = ring_read(&r, out, 8u);
    CHECK(n == 8u && out[0] == 5u && out[2] == 7u && out[3] == 10u && out[7] == 14u);
    CHECK(atomic_load(&r.head) < 0xFFFFFFFAu); /* 確實跨過了回繞點 */
}

/* ---- 2. ISR 生產、主迴圈消費 ---- */
#define SPSC_TOTAL 20000u
static ring_t g_spsc;
static uint8_t g_spsc_mem[16];
static uint32_t g_produced;
static uint8_t g_accepted[SPSC_TOTAL];
static uint32_t g_accepted_n;

static void isr_produce(void)
{
    if (g_produced >= SPSC_TOTAL) return;
    uint8_t v = (uint8_t)(g_produced * 7u + 3u);
    g_produced++;
    if (ring_push(&g_spsc, v)) g_accepted[g_accepted_n++] = v;
}

static void test_spsc_interleaving(void)
{
    printf("ISR 與主迴圈交錯：任何位置被打斷，資料都不重複、不錯序，遺失都被計數\n");
    ring_init(&g_spsc, g_spsc_mem, sizeof g_spsc_mem, 0xFFFFFF00u);
    g_produced = g_accepted_n = 0;
    g_isr = isr_produce;
    g_isr_chance = 3u;
    lcg_state = 11u;

    uint32_t consumed = 0, mismatches = 0;
    while (g_produced < SPSC_TOTAL || ring_count(&g_spsc)) {
        if (g_produced < SPSC_TOTAL && lcg() % 4u == 0u) {
            g_in_isr = 1;
            isr_produce(); /* 主迴圈沒在碰 ring 的時候也會有中斷 */
            g_in_isr = 0;
        }
        if (lcg() % 5u == 0u) continue; /* 主迴圈忙別的事，讓緩衝區有機會滿 */
        uint8_t b;
        if (ring_pop(&g_spsc, &b)) {
            if (consumed >= g_accepted_n || b != g_accepted[consumed]) mismatches++;
            consumed++;
        }
    }
    g_isr = NULL;
    printf("  產生 %u，收到 %u，丟棄 %u\n", (unsigned)g_produced, (unsigned)consumed,
           (unsigned)ring_dropped(&g_spsc));
    CHECK(mismatches == 0u);
    CHECK(consumed == g_accepted_n);
    CHECK(consumed + ring_dropped(&g_spsc) == SPSC_TOTAL);
    CHECK(ring_dropped(&g_spsc) > 0u); /* 這個測試刻意讓它滿過 */
}

/* ---- 3. 錯誤示範：共用 count ---- */
typedef struct {
    uint8_t buf[16];
    uint32_t w, r, count;
} bad_ring_t;
static bad_ring_t g_bad;
static uint32_t g_bad_produced, g_bad_dropped;
static uint8_t g_bad_log[SPSC_TOTAL];
static uint32_t g_bad_log_n;

static void bad_isr_produce(void)
{
    if (g_bad_produced >= SPSC_TOTAL) return;
    uint8_t v = (uint8_t)(g_bad_produced * 7u + 3u);
    g_bad_produced++;
    if (g_bad.count >= 16u) {
        g_bad_dropped++;
        return;
    }
    g_bad.buf[g_bad.w] = v;
    g_bad.w = (g_bad.w + 1u) % 16u;
    g_bad.count++; /* ISR 一次做完，不會被主程式打斷 */
    g_bad_log[g_bad_log_n++] = v;
}

static int bad_pop(uint8_t *b)
{
    if (g_bad.count == 0u) return 0;
    *b = g_bad.buf[g_bad.r];
    g_bad.r = (g_bad.r + 1u) % 16u;
    uint32_t c = g_bad.count; /* 讀 */
    ring_sim_preempt();        /* ISR 可能在這裡 count++ */
    g_bad.count = c - 1u;      /* 寫回：ISR 的遞增被覆蓋 */
    return 1;
}

static void test_shared_count_race(void)
{
    printf("錯誤示範：生產者和消費者都改同一個 count\n");
    memset(&g_bad, 0, sizeof g_bad);
    g_bad_produced = g_bad_dropped = g_bad_log_n = 0;
    g_isr = bad_isr_produce;
    g_isr_chance = 3u;
    lcg_state = 11u;
    uint32_t consumed = 0, mismatches = 0;
    for (uint32_t loops = 0; loops < 200000u && g_bad_produced < SPSC_TOTAL; loops++) {
        if (lcg() % 4u == 0u) {
            g_in_isr = 1;
            bad_isr_produce();
            g_in_isr = 0;
        }
        uint8_t b;
        if (bad_pop(&b)) {
            if (consumed >= g_bad_log_n || b != g_bad_log[consumed]) mismatches++;
            consumed++;
        }
    }
    g_isr = NULL;
    uint32_t real = (g_bad.w - g_bad.r + 16u) % 16u;
    printf("  count=%u，但讀寫位置相差 %u；錯序或重複 %u 次\n", (unsigned)g_bad.count, (unsigned)real,
           (unsigned)mismatches);
    CHECK(mismatches > 0u || g_bad.count != real);
}

/* ---- 4. CRC 與解析器 ---- */
static frame_t g_frames[256];
static uint32_t g_frame_n;
static void collect(void *ctx, const frame_t *f)
{
    (void)ctx;
    if (g_frame_n < 256u) g_frames[g_frame_n++] = *f;
}

static uint32_t make_sample(uint16_t seq, uint8_t *out)
{
    uint8_t p[SAMPLE_PAYLOAD];
    put_u16(&p[0], seq);
    put_u16(&p[2], (uint16_t)(seq * 10u));
    put_u16(&p[4], (uint16_t)(0xA5A5u)); /* 故意讓 payload 含有 SYNC 值 */
    p[6] = 0;
    return frame_encode(TYPE_SAMPLE, p, SAMPLE_PAYLOAD, out);
}

static int seqs_in_order(uint32_t first, uint32_t count)
{
    if (g_frame_n != count) return 0;
    for (uint32_t i = 0; i < count; i++)
        if (get_u16(g_frames[i].payload) != first + i) return 0;
    return 1;
}

static void test_parser(void)
{
    printf("CRC-16/CCITT-FALSE 檢查值\n");
    CHECK(crc16_ccitt_false((const uint8_t *)"123456789", 9u) == 0x29B1u);

    parser_t p;
    uint8_t stream[4096];
    uint32_t n = 0, starts[100];

    printf("封包之間夾雜隨機雜訊（包含 0xA5）\n");
    lcg_state = 5u;
    for (uint16_t s = 0; s < 100u; s++) {
        uint32_t noise = lcg() % 6u;
        for (uint32_t k = 0; k < noise; k++) stream[n++] = (k == 0) ? FRAME_SYNC : (uint8_t)lcg();
        starts[s] = n;
        n += make_sample(s, &stream[n]);
    }
    g_frame_n = 0;
    parser_init(&p, 20u, collect, NULL);
    for (uint32_t i = 0; i < n; i++) parser_feed(&p, stream[i], 0u);
    printf("  frames=%u crc錯=%u 過長=%u 跳過位元組=%u\n", (unsigned)p.frames_ok,
           (unsigned)p.err_crc, (unsigned)p.err_len, (unsigned)p.bytes_skipped);
    CHECK(seqs_in_order(0u, 100u));

    printf("在封包中途才開始接收：只損失第一個不完整的封包\n");
    g_frame_n = 0;
    parser_init(&p, 20u, collect, NULL);
    for (uint32_t i = starts[0] + 4u; i < n; i++) parser_feed(&p, stream[i], 0u);
    CHECK(seqs_in_order(1u, 99u));

    printf("改壞一個位元：該封包被拒絕，其餘照收\n");
    uint8_t saved = stream[starts[3] + 5u];
    stream[starts[3] + 5u] ^= 0x10u;
    g_frame_n = 0;
    parser_init(&p, 20u, collect, NULL);
    for (uint32_t i = 0; i < n; i++) parser_feed(&p, stream[i], 0u);
    stream[starts[3] + 5u] = saved;
    CHECK(p.err_crc >= 1u && g_frame_n == 99u && get_u16(g_frames[3].payload) == 4u);

    printf("長度欄位超過上限\n");
    uint8_t buf[64];
    g_frame_n = 0;
    parser_init(&p, 20u, collect, NULL);
    parser_feed(&p, FRAME_SYNC, 0u);
    parser_feed(&p, 0x40u, 0u);
    uint32_t m = make_sample(7u, buf);
    for (uint32_t i = 0; i < m; i++) parser_feed(&p, buf[i], 0u);
    CHECK(p.err_len == 1u && g_frame_n == 1u);

    printf("收到半個封包後停頓 100 ms：放棄半包，下一包正常\n");
    g_frame_n = 0;
    parser_init(&p, 20u, collect, NULL);
    for (uint32_t i = 0; i < 6u; i++) parser_feed(&p, buf[i], 0u);
    for (uint32_t i = 0; i < m; i++) parser_feed(&p, buf[i], 100u);
    CHECK(p.err_timeout == 1u && g_frame_n == 1u);
}

/* ================= 5. 整合模擬：裝置、UART、電腦 ================= */
typedef struct {
    int connected, busy;
    uint32_t n;
} sim_sensor_t;

static void sensor_start(void *ctx)
{
    sim_sensor_t *s = ctx;
    s->busy = 1;
    s->n++;
}
static int sensor_poll(void *ctx, int16_t *v)
{
    sim_sensor_t *s = ctx;
    if (!s->busy || !s->connected) return 0; /* 斷線時永遠不會完成 */
    s->busy = 0;
    *v = (int16_t)(1000 + (int)(s->n % 100u));
    return 1;
}

typedef struct {
    logger_t lg;
    host_rx_t host;
    sim_sensor_t sensor;
    uint32_t now_ms;
    uint32_t tx_bytes_per_s; /* 115200 baud、8N1：11520 */
    uint32_t tx_credit;      /* 以 1/1000 byte 為單位累積的傳送額度 */
    uint32_t rx_credit;
    uint8_t to_dev[1024];    /* 電腦要送給裝置的位元組 */
    uint32_t to_dev_head, to_dev_n;
    uint32_t main_blocked_until;
    uint32_t first_drop_ms;
} rig_t;

static rig_t g_rig; /* 很大，放在靜態儲存區而不是 stack */

static void rig_init(rig_t *r, uint32_t tx_start_index)
{
    memset(r, 0, sizeof *r);
    r->sensor.connected = 1;
    sensor_port_t port = {sensor_start, sensor_poll, &r->sensor};
    logger_init(&r->lg, port, 0u, tx_start_index);
    host_rx_init(&r->host);
    r->tx_bytes_per_s = 115200u / 10u;
}

static void rig_send(rig_t *r, uint8_t type, const uint8_t *payload, uint8_t len)
{
    uint8_t buf[FRAME_MAX_SIZE];
    uint32_t n = frame_encode(type, payload, len, buf);
    for (uint32_t i = 0; i < n && r->to_dev_n < sizeof r->to_dev; i++)
        r->to_dev[(r->to_dev_head + r->to_dev_n++) % sizeof r->to_dev] = buf[i];
}

static void rig_send_raw(rig_t *r, const uint8_t *buf, uint32_t n)
{
    for (uint32_t i = 0; i < n && r->to_dev_n < sizeof r->to_dev; i++)
        r->to_dev[(r->to_dev_head + r->to_dev_n++) % sizeof r->to_dev] = buf[i];
}

static void rig_step(rig_t *r)
{
    r->now_ms++;

    /* 電腦 → 裝置：每 ms 最多送出線路容量允許的位元組，每個位元組觸發一次 RX ISR */
    r->rx_credit += r->tx_bytes_per_s;
    while (r->to_dev_n && r->rx_credit >= 1000u) {
        uint8_t b = r->to_dev[r->to_dev_head];
        r->to_dev_head = (r->to_dev_head + 1u) % sizeof r->to_dev;
        r->to_dev_n--;
        r->rx_credit -= 1000u;
        logger_uart_rx_isr(&r->lg, b);
    }
    if (!r->to_dev_n) r->rx_credit = 0;

    if (r->now_ms >= r->main_blocked_until) logger_poll(&r->lg, r->now_ms);
    if (r->lg.dropped_frames && !r->first_drop_ms) r->first_drop_ms = r->now_ms;

    /* 裝置 → 電腦：TX ISR 依線路容量取出位元組 */
    r->tx_credit += r->tx_bytes_per_s;
    while (r->tx_credit >= 1000u) {
        uint8_t b;
        if (!logger_uart_tx_isr(&r->lg, &b)) {
            r->tx_credit = 0; /* 線路閒置時不會累積額度 */
            break;
        }
        r->tx_credit -= 1000u;
        host_rx_feed(&r->host, &b, 1u, r->now_ms);
    }
}

static void rig_run(rig_t *r, uint32_t ms)
{
    for (uint32_t i = 0; i < ms; i++) rig_step(r);
}

static void set_period(rig_t *r, uint16_t ms)
{
    uint8_t p[2];
    put_u16(p, ms);
    rig_send(r, TYPE_SET_PERIOD, p, 2u);
}

static void test_normal_run(void)
{
    printf("整合：10 ms 週期跑 10 秒\n");
    rig_t *r = &g_rig;
    rig_init(r, 0u);
    rig_run(r, 10005u); /* 多跑幾 ms，讓最後一個封包傳完 */
    printf("  電腦收到 %u 筆，遺失 %u，tick 間隔 %u..%u ms\n", (unsigned)r->host.samples,
           (unsigned)r->host.lost_samples, (unsigned)r->host.min_tick_delta,
           (unsigned)r->host.max_tick_delta);
    CHECK(r->host.samples == 1000u && r->host.lost_samples == 0u);
    CHECK(r->host.min_tick_delta == 10u && r->host.max_tick_delta == 10u);
    CHECK(parser_errors(&r->host.parser) == 0u);
}

static void test_commands(void)
{
    printf("命令：有效設定生效，無效設定被拒絕且保留原值\n");
    rig_t *r = &g_rig;
    rig_init(r, 0u);
    rig_run(r, 100u);
    set_period(r, 20u);
    rig_run(r, 50u);
    CHECK(r->host.acks == 1u && r->host.last_ack_cmd == TYPE_SET_PERIOD);
    uint32_t before = r->host.samples;
    r->host.min_tick_delta = 0xFFFFu;
    r->host.max_tick_delta = 0u;
    rig_run(r, 1000u);
    CHECK(r->host.samples - before == 50u && r->host.min_tick_delta == 20u &&
          r->host.max_tick_delta == 20u);

    set_period(r, 0u);
    rig_run(r, 20u);
    CHECK(r->host.nacks == 1u && r->host.last_ack_result == RESULT_OUT_OF_RANGE);
    set_period(r, 5000u);
    rig_run(r, 20u);
    CHECK(r->host.nacks == 2u && r->host.last_ack_result == RESULT_OUT_OF_RANGE);
    uint8_t one = 5u;
    rig_send(r, TYPE_SET_PERIOD, &one, 1u);
    rig_run(r, 20u);
    CHECK(r->host.nacks == 3u && r->host.last_ack_result == RESULT_BAD_LENGTH);
    rig_send(r, 0x55u, NULL, 0u);
    rig_run(r, 20u);
    CHECK(r->host.nacks == 4u && r->host.last_ack_result == RESULT_UNKNOWN_TYPE);
    rig_send(r, TYPE_GET_STATUS, NULL, 0u);
    rig_run(r, 20u);
    CHECK(r->host.statuses == 1u && r->host.status[0] == 20u);
}

static void test_overload(void)
{
    printf("超載：1 ms 週期 = 12,000 B/s，大於線路 11,520 B/s\n");
    rig_t *r = &g_rig;
    rig_init(r, 0u);
    set_period(r, 1u);
    rig_run(r, 5000u);
    /* 第一課：積壓每秒增加 480 bytes；可用空間約 1024 - 保留 16 - 一個封包 12 */
    double predicted = (double)(LOGGER_TX_CAPACITY - LOGGER_TX_RESERVE - SAMPLE_FRAME_SIZE) / 480.0;
    printf("  第一次丟棄在 %.2f s，預測約 %.2f s；裝置丟棄 %u 筆\n", r->first_drop_ms / 1000.0,
           predicted, (unsigned)r->lg.dropped_frames);
    CHECK(r->first_drop_ms > 1800u && r->first_drop_ms < 2400u);

    set_period(r, 1000u); /* 超載中送命令：靠保留空間仍能收到 ACK */
    uint32_t acks = r->host.acks;
    rig_run(r, 2500u);
    printf("  電腦算出遺失 %u 筆，帶 OVERFLOW 旗標的封包 %u 個\n", (unsigned)r->host.lost_samples,
           (unsigned)r->host.overflow_flags);
    CHECK(r->host.acks == acks + 1u);
    CHECK(r->host.lost_samples == r->lg.dropped_frames);
    CHECK(r->host.overflow_flags > 0u && r->lg.responses_dropped == 0u);
}

static void test_sensor_disconnect(void)
{
    printf("感測器在 3 s 到 4 s 之間斷線：取樣不停，封包標示逾時\n");
    rig_t *r = &g_rig;
    rig_init(r, 0u);
    rig_run(r, 3000u);
    r->sensor.connected = 0;
    rig_run(r, 1000u);
    r->sensor.connected = 1;
    rig_run(r, 6005u);
    printf("  收到 %u 筆，其中逾時 %u 筆，遺失 %u\n", (unsigned)r->host.samples,
           (unsigned)r->host.sensor_timeouts, (unsigned)r->host.lost_samples);
    CHECK(r->host.samples == 1000u && r->host.lost_samples == 0u);
    CHECK(r->host.sensor_timeouts >= 99u && r->host.sensor_timeouts <= 101u);
    CHECK(r->lg.missed_periods == 0u);
}

static void test_rx_faults(void)
{
    printf("命令只送一半就斷線，100 ms 後重新送完整命令\n");
    rig_t *r = &g_rig;
    rig_init(r, 0u);
    uint8_t half[3] = {FRAME_SYNC, 2u, TYPE_SET_PERIOD};
    rig_send_raw(r, half, 3u);
    rig_run(r, 100u);
    rig_send(r, TYPE_GET_STATUS, NULL, 0u);
    rig_run(r, 20u);
    CHECK(r->lg.parser.err_timeout == 1u && r->host.statuses == 1u);
    CHECK(r->lg.period_ms == 10u);

    printf("主迴圈卡住 30 ms，期間電腦送入 120 bytes：RX 緩衝區溢位被計數\n");
    rig_init(r, 0u);
    rig_run(r, 50u);
    r->main_blocked_until = r->now_ms + 30u;
    for (int i = 0; i < 24; i++) rig_send(r, TYPE_GET_STATUS, NULL, 0u); /* 24 x 5 bytes */
    rig_run(r, 200u);
    uint32_t statuses = r->host.statuses;
    rig_send(r, TYPE_GET_STATUS, NULL, 0u);
    rig_run(r, 20u);
    printf("  RX 溢位 %u bytes；回報的 rx_errors=%u\n", (unsigned)logger_rx_overflow(&r->lg),
           (unsigned)r->host.status[2]);
    CHECK(logger_rx_overflow(&r->lg) > 0u);
    CHECK(r->host.statuses == statuses + 1u && r->host.status[2] > 0u);
}

static void test_sequence_wrap(void)
{
    printf("長時間：2 ms 週期跑 140 s，序號（u16）與 TX 索引都會回繞\n");
    rig_t *r = &g_rig;
    rig_init(r, 0xFFFFF000u);
    set_period(r, 2u);
    rig_run(r, 140000u);
    printf("  收到 %u 筆，遺失 %u，TX head=0x%08X\n", (unsigned)r->host.samples,
           (unsigned)r->host.lost_samples, (unsigned)atomic_load(&r->lg.tx.head));
    CHECK(r->host.samples > 65536u && r->host.lost_samples == 0u);
    CHECK(atomic_load(&r->lg.tx.head) < 0xFFFFF000u);
}

int main(void)
{
    console_utf8();
#ifndef RING_SIMULATE_PREEMPTION
    printf("請加上 -DRING_SIMULATE_PREEMPTION 重新建置，交錯測試才會插入模擬中斷\n");
    return 2;
#endif
    test_ring_basics();
    test_spsc_interleaving();
    test_shared_count_race();
    test_parser();
    test_normal_run();
    test_commands();
    test_overload();
    test_sensor_disconnect();
    test_rx_faults();
    test_sequence_wrap();
    printf(g_fail ? "\n%d 項失敗\n" : "\n全部通過\n", g_fail);
    return g_fail ? 1 : 0;
}

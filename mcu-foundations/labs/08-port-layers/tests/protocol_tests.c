/*
 * protocol_tests.c：電腦端的協定測試。兩個平台共用，一個字都不改。
 *
 * 和第五課的 test_pipeline.c 不同，這裡完全不碰裝置內部：
 * 只送命令封包、收資料封包，用第五課的 host_rx.c 檢查序號與時間戳，
 * 裝置端的丟棄數、錯誤數一律用 GET_STATUS 從線路上問回來。
 * 所以它能拿來驗收任何一個「講同一種協定」的裝置，不管裡面是哪家的晶片。
 *
 * 建置（平台 A，一行）：
 *   gcc -std=c11 -Wall -Wextra -Werror -O2 -Icore -Itests -Iplat_a -I../04-uart-pipeline -o test_a tests/protocol_tests.c plat_a/sim_a.c plat_a/board_a.c core/app.c ../04-uart-pipeline/logger.c ../04-uart-pipeline/ring.c ../04-uart-pipeline/frame.c ../04-uart-pipeline/host_rx.c
 * 平台 B 把 plat_a、sim_a.c、board_a.c 換成 plat_b、sim_b.c、board_b.c。
 */
#include <stdio.h>
#include <string.h>

#include "frame.h"
#include "host_rx.h"
#include "sim.h"

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

static host_rx_t g_host;
static uint32_t g_first_gap_ms; /* 電腦第一次看到序號跳號的時間 */

static void sink(void *ctx, const uint8_t *data, uint32_t n, uint32_t now_ms)
{
    (void)ctx;
    host_rx_feed(&g_host, data, n, now_ms);
    if (!g_first_gap_ms && g_host.lost_samples) g_first_gap_ms = now_ms;
}

static void power_on(uint32_t time_origin_ms)
{
    host_rx_init(&g_host);
    g_first_gap_ms = 0;
    sim_reset(time_origin_ms, sink, NULL);
}

static void send(uint8_t type, const uint8_t *payload, uint8_t len)
{
    uint8_t buf[FRAME_MAX_SIZE];
    uint32_t n = frame_encode(type, payload, len, buf);
    sim_host_send(buf, n);
}

static void set_period(uint16_t ms)
{
    uint8_t p[2];
    put_u16(p, ms);
    send(TYPE_SET_PERIOD, p, 2u);
}

/* 問一次狀態並等回應；回傳是否收到新的 STATUS */
static int ask_status(void)
{
    uint32_t before = g_host.statuses;
    send(TYPE_GET_STATUS, NULL, 0u);
    sim_run_ms(30u);
    return g_host.statuses == before + 1u;
}

enum { ST_PERIOD, ST_DROPPED, ST_RX_ERRORS, ST_SENSOR_ERRORS, ST_MISSED };

static void test_normal(void)
{
    printf("10 ms 週期跑 10 秒：1000 筆、沒有跳號、時間戳間隔都是 10 ms\n");
    power_on(0u);
    sim_run_ms(10005u);
    printf("  電腦收到 %u 筆，遺失 %u，tick 間隔 %u..%u ms\n", (unsigned)g_host.samples,
           (unsigned)g_host.lost_samples, (unsigned)g_host.min_tick_delta,
           (unsigned)g_host.max_tick_delta);
    CHECK(g_host.samples == 1000u && g_host.lost_samples == 0u);
    CHECK(g_host.min_tick_delta == 10u && g_host.max_tick_delta == 10u);
    CHECK(parser_errors(&g_host.parser) == 0u);
}

static void test_commands(void)
{
    printf("命令：有效設定生效，無效設定被拒絕且保留原值\n");
    power_on(0u);
    sim_run_ms(100u);
    set_period(20u);
    sim_run_ms(50u);
    CHECK(g_host.acks == 1u && g_host.last_ack_cmd == TYPE_SET_PERIOD);
    uint32_t before = g_host.samples;
    g_host.min_tick_delta = 0xFFFFu;
    g_host.max_tick_delta = 0u;
    sim_run_ms(1000u);
    CHECK(g_host.samples - before == 50u && g_host.min_tick_delta == 20u &&
          g_host.max_tick_delta == 20u);

    set_period(0u);
    sim_run_ms(20u);
    CHECK(g_host.nacks == 1u && g_host.last_ack_result == RESULT_OUT_OF_RANGE);
    set_period(5000u);
    sim_run_ms(20u);
    CHECK(g_host.nacks == 2u && g_host.last_ack_result == RESULT_OUT_OF_RANGE);
    uint8_t one = 5u;
    send(TYPE_SET_PERIOD, &one, 1u);
    sim_run_ms(20u);
    CHECK(g_host.nacks == 3u && g_host.last_ack_result == RESULT_BAD_LENGTH);
    send(0x55u, NULL, 0u);
    sim_run_ms(20u);
    CHECK(g_host.nacks == 4u && g_host.last_ack_result == RESULT_UNKNOWN_TYPE);
    CHECK(ask_status() && g_host.status[ST_PERIOD] == 20u);
}

static void test_overload(void)
{
    printf("超載：1 ms 週期 = 12,000 B/s，大於線路的 11,520 B/s\n");
    power_on(0u);
    set_period(1u);
    sim_run_ms(5000u);
    printf("  電腦在 %.2f s 第一次看到跳號（裝置約在 2.08 s 開始丟，之後還要等緩衝區送完）\n",
           g_first_gap_ms / 1000.0);
    CHECK(g_first_gap_ms > 1800u && g_first_gap_ms < 2600u);

    uint32_t acks = g_host.acks;
    set_period(1000u); /* 超載中送命令：靠保留空間仍能收到 ACK */
    sim_run_ms(2500u);
    CHECK(g_host.acks == acks + 1u);
    CHECK(ask_status());
    printf("  電腦算出遺失 %u 筆，裝置回報丟棄 %u 筆，帶 OVERFLOW 的封包 %u 個\n",
           (unsigned)g_host.lost_samples, (unsigned)g_host.status[ST_DROPPED],
           (unsigned)g_host.overflow_flags);
    CHECK(g_host.lost_samples == g_host.status[ST_DROPPED] && g_host.lost_samples > 0u);
    CHECK(g_host.overflow_flags > 0u);
}

static void test_sensor_disconnect(void)
{
    printf("感測器在 3 s 到 4 s 之間斷線：取樣不停，封包標示逾時\n");
    power_on(0u);
    sim_run_ms(3000u);
    sim_sensor_connect(0);
    sim_run_ms(1000u);
    sim_sensor_connect(1);
    sim_run_ms(6005u);
    printf("  收到 %u 筆，其中逾時 %u 筆，遺失 %u\n", (unsigned)g_host.samples,
           (unsigned)g_host.sensor_timeouts, (unsigned)g_host.lost_samples);
    CHECK(g_host.samples == 1000u && g_host.lost_samples == 0u);
    CHECK(g_host.sensor_timeouts >= 99u && g_host.sensor_timeouts <= 101u);
    CHECK(ask_status() && g_host.status[ST_MISSED] == 0u &&
          g_host.status[ST_SENSOR_ERRORS] == g_host.sensor_timeouts);
}

static void test_rx_faults(void)
{
    printf("命令只送一半就停 100 ms：裝置放棄半包，之後的命令正常\n");
    power_on(0u);
    uint8_t half[3] = {FRAME_SYNC, 2u, TYPE_SET_PERIOD};
    sim_host_send(half, 3u);
    sim_run_ms(100u);
    CHECK(ask_status() && g_host.status[ST_PERIOD] == 10u && g_host.status[ST_RX_ERRORS] >= 1u);

    printf("主迴圈卡住 30 ms，期間電腦送入 120 bytes：接收溢位被計數，裝置照常回應\n");
    power_on(0u);
    sim_run_ms(50u);
    sim_block_main(30u);
    for (int i = 0; i < 24; i++) send(TYPE_GET_STATUS, NULL, 0u); /* 24 x 5 bytes */
    sim_run_ms(200u);
    CHECK(ask_status());
    printf("  回報的 rx_errors=%u\n", (unsigned)g_host.status[ST_RX_ERRORS]);
    CHECK(g_host.status[ST_RX_ERRORS] > 0u);
}

static void test_long_run(void)
{
    printf("長時間：毫秒計數從 0xFFFFF000 開始，2 ms 週期跑 140 s，毫秒與序號都會回繞\n");
    power_on(0xFFFFF000u);
    set_period(2u);
    sim_run_ms(100u);
    g_host.min_tick_delta = 0xFFFFu;
    g_host.max_tick_delta = 0u;
    sim_run_ms(140000u);
    printf("  收到 %u 筆，遺失 %u，tick 間隔 %u..%u ms\n", (unsigned)g_host.samples,
           (unsigned)g_host.lost_samples, (unsigned)g_host.min_tick_delta,
           (unsigned)g_host.max_tick_delta);
    CHECK(g_host.samples > 65536u && g_host.lost_samples == 0u);
    CHECK(g_host.min_tick_delta == 2u && g_host.max_tick_delta == 2u);
}

int main(void)
{
    console_utf8();
    printf("平台：%s\n\n", sim_platform());
    test_normal();
    test_commands();
    test_overload();
    test_sensor_disconnect();
    test_rx_faults();
    test_long_run();
    printf("\n");
    sim_print_stats();
    printf(g_fail ? "\n%d 項失敗\n" : "\n全部通過\n", g_fail);
    return g_fail ? 1 : 0;
}

/*
 * rx_check.c：讀取從序列埠存下來的原始位元組檔，檢查封包與序號。
 *
 * 建置：gcc -std=c11 -Wall -Wextra -Werror -O2 -o rx_check rx_check.c host_rx.c frame.c
 * 用法：
 *   ./rx_check capture.bin          分析檔案
 *   ./rx_check --make-demo demo.bin 產生一份含故障的示範檔（沒有板子時練習用）
 *   ./rx_check --make-cmd set-period 20 cmd.bin   產生要送給裝置的命令封包
 *   ./rx_check --make-cmd get-status cmd.bin
 *   ./rx_check --self-test          產生示範資料並檢查分析結果
 *
 * 示範資料的內容：200 筆 10 ms 取樣；序號 50、51、52 被丟棄（第 53 筆帶 OVERFLOW）；
 * 第 120 筆有一個位元錯誤；第 150 筆之前夾了 7 bytes 雜訊。
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "frame.h"
#include "host_rx.h"

static uint32_t make_demo(uint8_t *out, uint32_t cap)
{
    uint32_t n = 0;
    for (uint16_t seq = 0; seq < 200u; seq++) {
        if (seq >= 50u && seq <= 52u) continue; /* 裝置端 TX 緩衝區滿，被丟棄 */
        if (seq == 150u) {
            static const uint8_t noise[7] = {0x00, 0xA5, 0x07, 0x33, 0xA5, 0xFF, 0x10};
            memcpy(&out[n], noise, sizeof noise);
            n += sizeof noise;
        }
        uint8_t p[SAMPLE_PAYLOAD];
        put_u16(&p[0], seq);
        put_u16(&p[2], (uint16_t)(seq * 10u));
        put_u16(&p[4], (uint16_t)(1000 + seq % 50u));
        p[6] = seq == 53u ? STATUS_OVERFLOW : 0u;
        if (n + FRAME_MAX_SIZE > cap) break;
        uint32_t len = frame_encode(TYPE_SAMPLE, p, SAMPLE_PAYLOAD, &out[n]);
        if (seq == 120u) out[n + 7] ^= 0x04u; /* 傳輸中翻轉一個位元 */
        n += len;
    }
    return n;
}

/* 產生命令封包；回傳長度，參數錯誤回傳 0 */
static uint32_t make_cmd(const char *name, const char *arg, uint8_t *out)
{
    if (strcmp(name, "get-status") == 0) return frame_encode(TYPE_GET_STATUS, NULL, 0u, out);
    if (strcmp(name, "set-period") == 0 && arg) {
        char *end;
        unsigned long v = strtoul(arg, &end, 10);
        if (*end != '\0' || v > 0xFFFFu) return 0;
        uint8_t p[2];
        put_u16(p, (uint16_t)v); /* 範圍檢查刻意留給裝置端，才能測試 NACK */
        return frame_encode(TYPE_SET_PERIOD, p, 2u, out);
    }
    return 0;
}

static int write_file(const char *path, const uint8_t *data, uint32_t n)
{
    FILE *f = fopen(path, "wb");
    if (!f || fwrite(data, 1, n, f) != n) {
        perror(path);
        if (f) fclose(f);
        return 1;
    }
    fclose(f);
    printf("wrote %u bytes to %s\n", (unsigned)n, path);
    return 0;
}

static frame_t g_cmd_seen;
static void keep_frame(void *ctx, const frame_t *f)
{
    (void)ctx;
    g_cmd_seen = *f;
}

static void report(const host_rx_t *h, uint32_t bytes)
{
    const parser_t *p = &h->parser;
    printf("bytes             %u\n", (unsigned)bytes);
    printf("frames ok         %u\n", (unsigned)p->frames_ok);
    printf("samples           %u\n", (unsigned)h->samples);
    printf("seq gaps          %u events, %u samples missing\n", (unsigned)h->gap_events,
           (unsigned)h->lost_samples);
    printf("overflow flags    %u\n", (unsigned)h->overflow_flags);
    printf("sensor timeouts   %u\n", (unsigned)h->sensor_timeouts);
    printf("crc errors        %u\n", (unsigned)p->err_crc);
    printf("length errors     %u\n", (unsigned)p->err_len);
    printf("skipped bytes     %u\n", (unsigned)p->bytes_skipped);
    if (h->samples > 1u)
        printf("tick delta        %u..%u ms (consecutive samples only)\n",
               (unsigned)h->min_tick_delta, (unsigned)h->max_tick_delta);
}

static host_rx_t g_host;
static uint8_t g_buf[1u << 20];

static int self_test(void)
{
    uint32_t n = make_demo(g_buf, sizeof g_buf);
    host_rx_init(&g_host);
    host_rx_feed(&g_host, g_buf, n, 0u);
    report(&g_host, n);
    /* 197 個封包送出，其中 1 個 CRC 錯：收到 196 筆；遺失 = 被丟的 3 筆 + CRC 錯的 1 筆 */
    int ok = g_host.samples == 196u && g_host.lost_samples == 4u && g_host.gap_events == 2u &&
             g_host.overflow_flags == 1u && g_host.parser.err_crc >= 1u &&
             g_host.min_tick_delta == 10u && g_host.max_tick_delta == 10u;

    /* 命令封包：用同一個解析器解回來，確認 type 與 payload */
    uint8_t cmd[FRAME_MAX_SIZE];
    uint32_t m = make_cmd("set-period", "20", cmd);
    parser_t p;
    parser_init(&p, 1000u, keep_frame, NULL);
    for (uint32_t i = 0; i < m; i++) parser_feed(&p, cmd[i], 0u);
    ok = ok && m == 7u && p.frames_ok == 1u && g_cmd_seen.type == TYPE_SET_PERIOD &&
         get_u16(g_cmd_seen.payload) == 20u;
    ok = ok && make_cmd("get-status", NULL, cmd) == 5u && make_cmd("set-period", "abc", cmd) == 0u;
    printf(ok ? "self-test passed\n" : "self-test FAILED\n");
    return ok ? 0 : 1;
}

int main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "--self-test") == 0) return self_test();
    if (argc == 3 && strcmp(argv[1], "--make-demo") == 0)
        return write_file(argv[2], g_buf, make_demo(g_buf, sizeof g_buf));
    if ((argc == 4 || argc == 5) && strcmp(argv[1], "--make-cmd") == 0) {
        uint8_t cmd[FRAME_MAX_SIZE];
        uint32_t n = make_cmd(argv[2], argc == 5 ? argv[3] : NULL, cmd);
        if (n == 0) {
            printf("unknown command or bad value\n");
            return 2;
        }
        return write_file(argv[argc - 1], cmd, n);
    }
    if (argc != 2) {
        printf("usage: rx_check FILE | --make-demo FILE | --make-cmd set-period MS FILE | "
               "--make-cmd get-status FILE | --self-test\n");
        return 2;
    }
    FILE *f = fopen(argv[1], "rb");
    if (!f) {
        perror(argv[1]);
        return 1;
    }
    size_t n = fread(g_buf, 1, sizeof g_buf, f);
    fclose(f);
    if (n == sizeof g_buf) printf("note: only the first %u bytes are analysed\n", (unsigned)n);
    host_rx_init(&g_host);
    host_rx_feed(&g_host, g_buf, (uint32_t)n, 0u);
    report(&g_host, (uint32_t)n);
    return 0;
}

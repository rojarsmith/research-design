/*
 * uart_dma.c：UART 接收交給循環 DMA，CPU 只在半滿（HT）、全滿（TC）、
 * 線路閒置（IDLE）或定時檢查時，把新到的位元組一次搬走。
 * 沒有適當的 ADC 時，先用這個練習 DMA 的 buffer 所有權與批次處理。
 *
 * 第五課的 ring.c 與 frame.c 原封不動地拿來用：DMA 換掉的只是「每個 byte 進一次 RX 中斷」，
 * 解析器和環形緩衝區都不必改。
 *
 * 時間單位是奈秒，全部由程式推進，每次執行結果都相同。115,200 baud、8N1，
 * 每個 byte 約 86.8 µs。DMA buffer 64 bytes，半滿與全滿各在 32 與 64 bytes 時發出事件。
 *
 * 所有權：
 *   DMA 從 last_pos 往後寫；[last_pos, pos) 是已經寫好、還沒搬走的資料。
 *   service() 把這一段複製進 ring.c，然後 last_pos = pos，這一段就還給 DMA。
 *   DMA 只回報 pos，不回報繞了幾圈。兩次 service 之間到了 64 個以上的 byte，
 *   pos 看起來和沒收到東西一樣，資料就被無聲覆寫。
 *
 * 一圈檢查：有 1 ms 定時檢查時，正常的 service 間隔不超過 1 ms；
 *   間隔超過 63 個 byte 的時間，代表中斷被擋太久，buffer 內容不可信，
 *   整段丟掉並計數，交給協定層重新同步。
 *
 * 模型假設所有呼叫 service() 的中斷優先權相同，不會互相打斷。
 *
 * 建置（一行）：
 *   gcc -std=c11 -Wall -Wextra -Werror -O2 -I../04-uart-pipeline -o uart_dma uart_dma.c ../04-uart-pipeline/ring.c ../04-uart-pipeline/frame.c
 * 執行：./uart_dma                 印出三組實驗的結果
 *       ./uart_dma --fault=NAME    注入一種錯誤再跑一次，NAME 見 --help
 *       ./uart_dma --test          執行自我檢查
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "frame.h"
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

#define BYTE_NS 86806u    /* 10 bits / 115200 baud */
#define DMA_N 64u
#define TICK_NS 1000000u  /* 1 ms 定時檢查 */
#define NEVER UINT64_MAX
#define MAX_BYTES 40000u

typedef enum { FAULT_NONE, FAULT_NO_WRAP, FAULT_NO_GAP_CHECK, FAULT_COUNT } fault_t;
static const char *const fault_names[FAULT_COUNT] = {"none", "no-wrap", "no-gap-check"};

typedef struct {
    const char *name;
    int idle;  /* 線路閒置事件 */
    int tick;  /* 1 ms 定時檢查 */
} mode_t_cfg;

static const mode_t_cfg modes[] = {
    {"ht+tc", 0, 0},
    {"idle", 1, 0},
    {"tick", 0, 1},
    {"idle+tick", 1, 1},
};
#define MODE_COUNT (sizeof modes / sizeof modes[0])

/* ---------------- 電腦送出的位元組流 ---------------- */
static uint8_t tx_byte[MAX_BYTES];
static uint64_t tx_done_ns[MAX_BYTES]; /* 這個 byte 的停止位結束的時刻 */
static uint32_t tx_n;
static uint64_t tx_t; /* 下一個 byte 可以開始送的時刻 */

static void host_reset(void)
{
    tx_n = 0;
    tx_t = 0;
}

static void host_gap(uint64_t ns) { tx_t += ns; }

static void host_send(const uint8_t *buf, uint32_t n)
{
    for (uint32_t i = 0; i < n && tx_n < MAX_BYTES; i++) {
        tx_t += BYTE_NS;
        tx_byte[tx_n] = buf[i];
        tx_done_ns[tx_n++] = tx_t;
    }
}

/* 測試用的封包：payload 開頭放序號，裝置端用它檢查有沒有掉封包 */
static void host_send_seq(uint8_t type, uint16_t seq, uint8_t len)
{
    uint8_t p[FRAME_MAX_PAYLOAD] = {0};
    put_u16(p, seq);
    for (uint8_t i = 2; i < len; i++) p[i] = (uint8_t)(seq * 7u + i);
    uint8_t buf[FRAME_MAX_SIZE];
    host_send(buf, frame_encode(type, p, len, buf));
}

/* ---------------- 裝置端 ---------------- */
typedef struct {
    uint64_t start, len;
} mask_t;

typedef struct {
    uint32_t frames, seq_gaps, crc_err, isr_runs, lap_events;
    uint32_t written, delivered, stuck, truth_lost, suspect_max;
    uint64_t first_frame_ns; /* 第一個封包被解析完成的時刻 */
    uint64_t max_gap_ns;     /* 兩次 service 之間最長的間隔 */
} stats_t;

static uint8_t dma_buf[DMA_N];
static uint32_t dma_pos;     /* 硬體：DMA 下一個要寫的位置（相當於從剩餘計數換算） */
static uint32_t last_pos;    /* 韌體：上次搬到哪裡 */
static uint64_t last_service_ns;
static uint32_t written_at_service;
static ring_t rx_ring;
static uint8_t rx_mem[256];
static parser_t parser;
static int have_seq;
static uint16_t last_seq;
static stats_t st;
static const mode_t_cfg *md;
static fault_t g_fault;
static uint64_t now_ns;

static void on_frame(void *ctx, const frame_t *f)
{
    (void)ctx;
    st.frames++;
    if (!st.first_frame_ns) st.first_frame_ns = now_ns;
    if (f->len < 2u) return;
    uint16_t seq = get_u16(f->payload);
    if (have_seq && (uint16_t)(seq - last_seq) != 1u) st.seq_gaps += (uint16_t)(seq - last_seq - 1u);
    have_seq = 1;
    last_seq = seq;
}

static void copy_out(uint32_t from, uint32_t to)
{
    for (uint32_t i = from; i < to; i++) {
        ring_push(&rx_ring, dma_buf[i]);
        st.delivered++;
    }
}

/* 中斷裡執行：把 [last_pos, pos) 搬進環形緩衝區 */
static void service(void)
{
    st.isr_runs++;
    uint32_t pos = dma_pos;
    uint64_t gap = now_ns - last_service_ns;
    if (gap > st.max_gap_ns) st.max_gap_ns = gap;
    if (md->tick && g_fault != FAULT_NO_GAP_CHECK && gap >= (uint64_t)(DMA_N - 1u) * BYTE_NS) {
        /* 被擋得比一圈還久：pos 看不出繞了幾圈，整段丟掉並計數 */
        st.lap_events++;
        st.suspect_max += (uint32_t)(gap / BYTE_NS) + 1u;
    } else if (pos >= last_pos) {
        copy_out(last_pos, pos);
    } else if (g_fault == FAULT_NO_WRAP) {
        copy_out(0u, pos); /* 錯誤：忘了 buffer 尾端那一段 */
    } else {
        copy_out(last_pos, DMA_N); /* 繞過結尾：分兩段搬 */
        copy_out(0u, pos);
    }
    last_pos = pos;
    last_service_ns = now_ns;
    written_at_service = st.written;
}

/* 主迴圈：從環形緩衝區取出位元組交給解析器（模型裡緊接在 service 之後執行） */
static void main_loop_drain(void)
{
    uint8_t b;
    while (ring_pop(&rx_ring, &b)) parser_feed(&parser, b, (uint32_t)(now_ns / 1000000u));
}

static int masked, pending;

static void raise_irq(void)
{
    if (masked) {
        pending = 1; /* pending 是一個狀態，不是計數器：被擋住的多次事件只會進一次 */
        return;
    }
    service();
    main_loop_drain();
}

static stats_t run(const mode_t_cfg *m, fault_t fault, const mask_t *masks, uint32_t nmask,
                   uint64_t end_ns)
{
    md = m;
    g_fault = fault;
    memset(&st, 0, sizeof st);
    memset(dma_buf, 0, sizeof dma_buf);
    dma_pos = last_pos = 0;
    last_service_ns = 0;
    written_at_service = 0;
    ring_init(&rx_ring, rx_mem, sizeof rx_mem, 0u);
    parser_init(&parser, 20u, on_frame, NULL);
    have_seq = 0;
    masked = pending = 0;
    now_ns = 0;

    uint32_t bi = 0, mi = 0;
    uint64_t idle_at = NEVER;
    uint64_t next_tick = m->tick ? TICK_NS : NEVER;
    uint64_t unmask_at = NEVER;
    for (;;) {
        uint64_t tb = bi < tx_n ? tx_done_ns[bi] : NEVER;
        uint64_t tm = mi < nmask ? masks[mi].start : NEVER;
        uint64_t t = tb;
        if (idle_at < t) t = idle_at;
        if (next_tick < t) t = next_tick;
        if (tm < t) t = tm;
        if (unmask_at < t) t = unmask_at;
        if (t == NEVER || t > end_ns) break;
        now_ns = t;

        if (t == unmask_at) {
            masked = 0;
            unmask_at = NEVER;
            if (pending) {
                pending = 0;
                service();
                main_loop_drain();
            }
        } else if (t == tm) {
            masked = 1;
            unmask_at = t + masks[mi].len;
            mi++;
        } else if (t == tb) {
            /* 一個 byte 收完：DMA 把它寫進 buffer，CPU 不參與 */
            dma_buf[dma_pos] = tx_byte[bi++];
            st.written++;
            dma_pos = (dma_pos + 1u) % DMA_N;
            if (m->idle) idle_at = t + BYTE_NS;
            if (dma_pos == DMA_N / 2u || dma_pos == 0u) raise_irq(); /* HT 或 TC */
        } else if (t == idle_at) {
            idle_at = NEVER;
            /* 下一個 byte 的起始位已經開始，線路不算閒置 */
            if (!(bi < tx_n && tx_done_ns[bi] - BYTE_NS < t)) raise_irq();
        } else {
            next_tick += TICK_NS;
            raise_irq();
        }
    }
    st.crc_err = parser.err_crc;
    st.stuck = st.written - written_at_service;
    st.truth_lost = st.written - st.delivered - st.stuck;
    return st;
}

/* ---------------- 三組實驗 ---------------- */
/* 1. 一個 5-byte 命令之後安靜 100 ms */
static void traffic_single(void)
{
    host_reset();
    host_gap(1000000u);
    uint8_t buf[FRAME_MAX_SIZE];
    host_send(buf, frame_encode(TYPE_GET_STATUS, NULL, 0u, buf));
}

/* 2. 300 個長度不同的封包，間隔 0 到 3 個 byte 時間 */
static void traffic_mixed(void)
{
    host_reset();
    uint32_t lcg = 9u;
    for (uint16_t s = 0; s < 300u; s++) {
        lcg = lcg * 1664525u + 1013904223u;
        host_gap((uint64_t)((lcg >> 24) % 4u) * BYTE_NS);
        if ((lcg >> 16) & 1u)
            host_send_seq(TYPE_SET_PERIOD, s, 2u);      /* 7 bytes */
        else
            host_send_seq(TYPE_SAMPLE, s, SAMPLE_PAYLOAD); /* 12 bytes */
    }
    host_gap(10000000u);
}

/* 3. 2000 個 12-byte 封包連續送，約 2.1 秒 */
static void traffic_stream(void)
{
    host_reset();
    for (uint16_t s = 0; s < 2000u; s++) host_send_seq(TYPE_SAMPLE, s, SAMPLE_PAYLOAD);
    host_gap(10000000u);
}

static const uint64_t END_NS = 3000000000ull;

static void print_all(fault_t f)
{
    printf("UART RX 循環 DMA，buffer %u bytes，115200 8N1；故障：%s\n\n", DMA_N, fault_names[f]);

    traffic_single();
    uint64_t last_byte = tx_done_ns[tx_n - 1u];
    printf("1. 一個 5-byte 命令，之後線路安靜 100 ms\n");
    printf("   %-10s %-8s %s\n", "mode", "收到", "最後一個 byte 到解析完成（us）");
    for (size_t i = 0; i < MODE_COUNT; i++) {
        stats_t r = run(&modes[i], f, NULL, 0u, last_byte + 100000000u);
        if (r.frames)
            printf("   %-10s %-8s %.1f\n", modes[i].name, "是", (r.first_frame_ns - last_byte) / 1000.0);
        else
            printf("   %-10s %-8s 卡在 DMA buffer 裡 %u bytes\n", modes[i].name, "否", (unsigned)r.stuck);
    }

    traffic_mixed();
    printf("\n2. 300 個 7 或 12 bytes 的封包，間隔 0 到 3 個 byte 時間\n");
    printf("   %-10s %6s %8s %7s %6s %8s\n", "mode", "frames", "seq_gaps", "crc_err", "stuck", "isr_runs");
    for (size_t i = 0; i < MODE_COUNT; i++) {
        stats_t r = run(&modes[i], f, NULL, 0u, END_NS);
        printf("   %-10s %6u %8u %7u %6u %8u\n", modes[i].name, (unsigned)r.frames,
               (unsigned)r.seq_gaps, (unsigned)r.crc_err, (unsigned)r.stuck, (unsigned)r.isr_runs);
    }

    traffic_stream();
    printf("\n3. 2000 個 12-byte 封包連續送；idle+tick；0.5 秒時擋中斷 3 ms，1.0 秒時擋 8 ms\n");
    const mask_t masks[2] = {{500000000ull, 3000000ull}, {1000000000ull, 8000000ull}};
    stats_t r = run(&modes[3], f, masks, 2u, END_NS);
    printf("   frames=%u seq_gaps=%u crc_err=%u lap_events=%u suspect_max=%u truth_lost=%u max_gap=%.2f ms\n",
           (unsigned)r.frames, (unsigned)r.seq_gaps, (unsigned)r.crc_err, (unsigned)r.lap_events,
           (unsigned)r.suspect_max, (unsigned)r.truth_lost, r.max_gap_ns / 1e6);
    printf("\nstuck=最後一次 service 之後還留在 DMA buffer 的 bytes  lap_events=韌體發現被擋超過一圈的次數\n"
           "suspect_max=韌體估計最多可能遺失的 bytes  truth_lost=真相：沒有被搬走的 bytes\n");
}

/* ---------------- 自我檢查 ---------------- */
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

static int run_tests(void)
{
    printf("單一短命令：只有 HT/TC 時卡在 buffer 裡；IDLE 在一個 byte 時間後就交出去\n");
    traffic_single();
    uint64_t last_byte = tx_done_ns[tx_n - 1u];
    stats_t a = run(&modes[0], FAULT_NONE, NULL, 0u, last_byte + 100000000u);
    stats_t b = run(&modes[1], FAULT_NONE, NULL, 0u, last_byte + 100000000u);
    stats_t c = run(&modes[2], FAULT_NONE, NULL, 0u, last_byte + 100000000u);
    CHECK(a.frames == 0u && a.stuck == 5u);
    CHECK(b.frames == 1u && b.first_frame_ns - last_byte <= 2u * BYTE_NS);
    CHECK(c.frames == 1u && c.first_frame_ns - last_byte <= TICK_NS);

    printf("300 個長度不同的封包：跨過 buffer 結尾也完整，序號連續\n");
    traffic_mixed();
    stats_t m = run(&modes[1], FAULT_NONE, NULL, 0u, END_NS);
    CHECK(m.frames == 300u && m.seq_gaps == 0u && m.crc_err == 0u && m.stuck == 0u);
    CHECK(m.delivered == m.written);
    stats_t h = run(&modes[0], FAULT_NONE, NULL, 0u, END_NS);
    printf("  只有 HT/TC：收到 %u 個，%u bytes 卡在 buffer 裡等下一次半滿\n", (unsigned)h.frames,
           (unsigned)h.stuck);
    CHECK(h.frames < 300u && h.stuck > 0u && h.seq_gaps == 0u);

    printf("故障 no-wrap：跨過 buffer 結尾的那一段被漏搬\n");
    stats_t w = run(&modes[1], FAULT_NO_WRAP, NULL, 0u, END_NS);
    printf("  frames=%u seq_gaps=%u crc_err=%u\n", (unsigned)w.frames, (unsigned)w.seq_gaps,
           (unsigned)w.crc_err);
    CHECK(w.frames < 300u && w.delivered < w.written);

    printf("連續資料流，擋中斷 3 ms（不到一圈的 5.6 ms）：被擋住的幾次事件合併成一次搬運，仍然沒有遺失\n");
    traffic_stream();
    const mask_t short_mask = {500000000ull, 3000000ull};
    stats_t s = run(&modes[3], FAULT_NONE, &short_mask, 1u, END_NS);
    CHECK(s.frames == 2000u && s.seq_gaps == 0u && s.truth_lost == 0u && s.lap_events == 0u);

    printf("擋中斷 8 ms（超過一圈）：韌體發現並計數，估計值不少於真正遺失的 bytes\n");
    const mask_t long_mask = {1000000000ull, 8000000ull};
    stats_t l = run(&modes[3], FAULT_NONE, &long_mask, 1u, END_NS);
    printf("  lap_events=%u suspect_max=%u truth_lost=%u seq_gaps=%u frames=%u\n", (unsigned)l.lap_events,
           (unsigned)l.suspect_max, (unsigned)l.truth_lost, (unsigned)l.seq_gaps, (unsigned)l.frames);
    CHECK(l.lap_events == 1u && l.truth_lost > 0u && l.suspect_max >= l.truth_lost);
    CHECK(l.seq_gaps > 0u && l.frames + l.seq_gaps == 2000u); /* 解析器重新同步，後面的封包都收到 */

    printf("故障 no-gap-check：同樣擋 8 ms，DMA 這一層回報零遺失，只有協定層的序號看得出來\n");
    stats_t g = run(&modes[3], FAULT_NO_GAP_CHECK, &long_mask, 1u, END_NS);
    printf("  lap_events=%u truth_lost=%u seq_gaps=%u\n", (unsigned)g.lap_events, (unsigned)g.truth_lost,
           (unsigned)g.seq_gaps);
    CHECK(g.lap_events == 0u && g.truth_lost > 0u && g.seq_gaps > 0u);

    printf(g_fail ? "\n%d 項失敗\n" : "\n全部通過\n", g_fail);
    return g_fail ? 1 : 0;
}

static void usage(void)
{
    printf("用法：uart_dma [--test] [--fault=NAME]\n故障名稱：");
    for (int i = 1; i < FAULT_COUNT; i++) printf(" %s", fault_names[i]);
    printf("\n");
}

int main(int argc, char **argv)
{
    console_utf8();
    fault_t f = FAULT_NONE;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--test") == 0) return run_tests();
        if (strncmp(argv[i], "--fault=", 8) == 0) {
            int found = 0;
            for (int k = 0; k < FAULT_COUNT; k++)
                if (strcmp(argv[i] + 8, fault_names[k]) == 0) {
                    f = (fault_t)k;
                    found = 1;
                }
            if (!found) {
                usage();
                return 2;
            }
        } else {
            usage();
            return strcmp(argv[i], "--help") == 0 ? 0 : 2;
        }
    }
    print_all(f);
    return 0;
}

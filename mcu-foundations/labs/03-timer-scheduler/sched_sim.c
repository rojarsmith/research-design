/*
 * sched_sim.c：用虛擬時間模擬 tick 中斷、非阻塞主迴圈、去彈跳與計數器回繞。
 *
 * 時間單位是微秒，全部由程式推進，不讀桌機時鐘，所以每次執行結果都相同。
 * 模型裡的成本數字（ISR 2 µs、取樣 50 µs……）是假設，用來比較設計，
 * 不代表任何晶片的實測值；上板後要換成你自己量到的數字。
 *
 * 建置：gcc -std=c11 -Wall -Wextra -Werror -O2 -o sched_sim sched_sim.c
 * 執行：./sched_sim          印出各情境的統計
 *       ./sched_sim --test   執行自我檢查
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

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

/* ================= 1. 計數器回繞 ================= */

/* 錯誤寫法：比較絕對時間。deadline 回繞成小數字時，now 仍很大，條件立刻成立 */
static int due_naive(uint32_t now, uint32_t deadline) { return now >= deadline; }

/* 正確寫法：unsigned 減法有模數語意，經過時間在回繞前後都算得對 */
static int due_elapsed(uint32_t now, uint32_t last, uint32_t period)
{
    return (uint32_t)(now - last) >= period;
}

/* 從 start 開始跑 duration 個 tick，回傳 10 ms 任務實際執行次數 */
static int count_runs_naive(uint32_t start, uint32_t duration)
{
    uint32_t deadline = start + 10u;
    int runs = 0;
    for (uint32_t i = 0; i <= duration; i++) {
        uint32_t now = start + i;
        if (due_naive(now, deadline)) {
            runs++;
            deadline += 10u;
        }
    }
    return runs;
}

static int count_runs_elapsed(uint32_t start, uint32_t duration)
{
    uint32_t last = start;
    int runs = 0;
    for (uint32_t i = 0; i <= duration; i++) {
        uint32_t now = start + i;
        if (due_elapsed(now, last, 10u)) {
            runs++;
            last += 10u;
        }
    }
    return runs;
}

/* ================= 2. 撕裂讀取（torn read） ================= */
/* 假設一顆 16 位元 MCU：32 位元的 tick 要分兩次讀，中斷可能插在兩次之間 */
static uint32_t g_tick32;
static int g_isr_between_halves;

static void tick_isr_simple(void) { g_tick32++; }

static uint16_t bus_read16(int high)
{
    uint16_t v = high ? (uint16_t)(g_tick32 >> 16) : (uint16_t)g_tick32;
    if (g_isr_between_halves) { /* 中斷剛好在這次讀取之後發生 */
        g_isr_between_halves = 0;
        tick_isr_simple();
    }
    return v;
}

static uint32_t read_ticks_torn(void)
{
    uint16_t lo = bus_read16(0);
    uint16_t hi = bus_read16(1);
    return (uint32_t)hi << 16 | lo;
}

/* 先讀高半、再讀低半、再讀一次高半；高半沒變才採用，否則重讀 */
static uint32_t read_ticks_safe(void)
{
    for (;;) {
        uint16_t hi1 = bus_read16(1);
        uint16_t lo = bus_read16(0);
        uint16_t hi2 = bus_read16(1);
        if (hi1 == hi2) return (uint32_t)hi1 << 16 | lo;
    }
}

/* ================= 3. 主迴圈排程模擬 ================= */
enum { POLICY_CATCH_UP, POLICY_SKIP, POLICY_REBASE };
static const char *const policy_names[] = {"catch-up", "skip+count", "rebase"};

#define TICK_US 1000u
#define TICK_ISR_US 2u
#define SAMPLE_US 50u
#define SAMPLE_IN_ISR_US 20u
#define CMD_US 150u
#define LOOP_US 3u
#define PERIOD_MS 10u
#define LED_PERIOD_MS 500u

typedef struct {
    const char *name;
    int policy;
    int sample_in_isr;      /* 1：在 tick ISR 裡取樣 */
    uint32_t led_block_us;  /* LED 任務用 busy wait 的時間；0 表示非阻塞 */
    uint32_t masked_us;     /* 命令處理時關中斷的時間 */
    uint32_t sim_ms;
} scenario_t;

typedef struct {
    uint32_t samples;
    uint32_t missed_counted;   /* 排程器自己承認錯過的次數 */
    uint32_t max_late_us;
    uint64_t sum_late_us;
    uint32_t ticks_lost;       /* 關中斷太久，兩次 tick 合併成一次 */
    uint32_t max_cmd_wait_us;  /* 命令到達到開始處理的最長等待 */
    uint32_t commands;
} stats_t;

static const scenario_t *sc;
static stats_t st;
static uint64_t t_now;          /* 目前虛擬時間（µs） */
static uint64_t next_tick_at;   /* 下一次 tick 事件的時間 */
static volatile uint32_t g_ticks;
static uint32_t isr_div;
static uint64_t isr_slot_us;    /* ISR 取樣：下一個理想取樣時間 */

static uint64_t sample_limit_us; /* 只統計理想時間落在模擬期間內的樣本 */

static void record_sample(uint64_t ideal_us)
{
    if (ideal_us > sample_limit_us) return;
    uint32_t late = (uint32_t)(t_now - ideal_us);
    st.samples++;
    st.sum_late_us += late;
    if (late > st.max_late_us) st.max_late_us = late;
}

/* tick ISR：在 t_now 開始執行，回傳這次 ISR 花掉的時間 */
static uint64_t tick_isr(void)
{
    uint64_t cost = TICK_ISR_US;
    g_ticks++;
    if (sc->sample_in_isr && ++isr_div >= PERIOD_MS) {
        isr_div = 0;
        isr_slot_us += (uint64_t)PERIOD_MS * TICK_US;
        record_sample(isr_slot_us);
        cost += SAMPLE_IN_ISR_US;
    }
    return cost;
}

/* 主程式花 us 微秒做事；期間到期的 tick 會插進來執行，並把主程式往後推 */
static void cpu_spend(uint64_t us)
{
    uint64_t end = t_now + us;
    while (next_tick_at <= end) {
        t_now = next_tick_at;
        next_tick_at += TICK_US;
        uint64_t c = tick_isr();
        t_now += c;
        end += c;
    }
    t_now = end;
}

/* 關中斷做事：期間的 tick 只能掛起。pending 是一個狀態位元，不是計數器 */
static void cpu_spend_masked(uint64_t us)
{
    uint64_t end = t_now + us;
    uint32_t fired = 0;
    while (next_tick_at <= end) {
        fired++;
        next_tick_at += TICK_US;
    }
    t_now = end;
    if (fired) {
        st.ticks_lost += fired - 1u;
        t_now += tick_isr(); /* 開中斷後立刻進入 ISR，但只會進一次 */
    }
}

static uint32_t lcg_state;
static uint32_t lcg(void)
{
    lcg_state = lcg_state * 1664525u + 1013904223u;
    return lcg_state >> 8;
}

static stats_t run_scenario(const scenario_t *s)
{
    sc = s;
    memset(&st, 0, sizeof st);
    t_now = 0;
    next_tick_at = TICK_US;
    g_ticks = 0;
    isr_div = 0;
    isr_slot_us = 0;
    lcg_state = 42u;

    uint32_t sample_last = 0, led_last = 0;
    uint64_t cmd_at = 3000u + lcg() % 8000u;
    /* 多跑半個週期，讓最後一個期限（第 sim_ms 個 tick）也有機會被處理 */
    uint64_t end_us = (uint64_t)s->sim_ms * TICK_US + PERIOD_MS * TICK_US / 2u;
    sample_limit_us = (uint64_t)s->sim_ms * TICK_US;

    while (t_now < end_us) {
        uint32_t now = g_ticks; /* 32 位元核心上對齊的 32 位元讀取一次完成（假設） */

        if (!s->sample_in_isr && due_elapsed(now, sample_last, PERIOD_MS)) {
            uint32_t periods = (uint32_t)(now - sample_last) / PERIOD_MS; /* 已到期幾期 */
            uint32_t slot;
            switch (s->policy) {
            case POLICY_CATCH_UP: /* 一次補一筆最舊的期，期限只往前推一個週期 */
                slot = sample_last + PERIOD_MS;
                sample_last = slot;
                break;
            case POLICY_SKIP: /* 只做最新的一期，保持相位，跳過的期數記下來 */
                slot = sample_last + periods * PERIOD_MS;
                st.missed_counted += periods - 1u;
                sample_last = slot;
                break;
            default: /* REBASE：從現在重新起算，錯過的期數無聲消失 */
                slot = sample_last + PERIOD_MS;
                sample_last = now;
                break;
            }
            record_sample((uint64_t)slot * TICK_US);
            cpu_spend(SAMPLE_US);
        }

        if (due_elapsed(now, led_last, LED_PERIOD_MS)) {
            led_last += LED_PERIOD_MS;
            cpu_spend(s->led_block_us ? s->led_block_us : 5u);
        }

        if (cmd_at <= t_now) {
            uint32_t wait = (uint32_t)(t_now - cmd_at);
            if (wait > st.max_cmd_wait_us) st.max_cmd_wait_us = wait;
            st.commands++;
            cpu_spend(CMD_US);
            if (s->masked_us) cpu_spend_masked(s->masked_us);
            cmd_at = t_now + 3000u + lcg() % 8000u;
        }

        cpu_spend(LOOP_US);
    }
    return st;
}

static void print_stats(char id, const scenario_t *s, const stats_t *r)
{
    uint32_t expected = s->sim_ms / PERIOD_MS;
    printf("%-4c %-10s %5u/%-5u %6u %9u %8u %6u %9u\n", id,
           s->sample_in_isr ? "isr" : policy_names[s->policy], (unsigned)r->samples,
           (unsigned)expected, (unsigned)r->missed_counted, (unsigned)r->max_late_us,
           r->samples ? (unsigned)(r->sum_late_us / r->samples) : 0u, (unsigned)r->ticks_lost,
           (unsigned)r->max_cmd_wait_us);
}

/* ================= 4. 按鈕去彈跳 ================= */
#define DEBOUNCE_MS 10u

typedef struct {
    int stable;      /* 目前認定的狀態：1 放開、0 按下（上拉，按下為低） */
    uint32_t count;  /* 原始輸入和 stable 不同已連續幾個 ms */
} debounce_t;

/* 每 1 ms 呼叫一次；回傳 +1 按下事件、-1 放開事件、0 沒有事件 */
static int debounce_step(debounce_t *d, int raw)
{
    if (raw == d->stable) {
        d->count = 0;
        return 0;
    }
    if (++d->count < DEBOUNCE_MS) return 0;
    d->stable = raw;
    d->count = 0;
    return raw == 0 ? +1 : -1;
}

/* 產生按鈕原始波形：每次按下與放開的前 bounce_ms 毫秒隨機跳動 */
typedef struct {
    int presses, naive_edges, debounced_press, debounced_release;
} button_result_t;

static button_result_t simulate_button(int presses, uint32_t hold_ms, uint32_t bounce_ms)
{
    button_result_t r = {presses, 0, 0, 0};
    debounce_t d = {1, 0};
    int prev_raw = 1;
    lcg_state = 7u;
    for (int p = 0; p < presses; p++) {
        for (int phase = 0; phase < 2; phase++) {
            int target = phase == 0 ? 0 : 1;
            uint32_t len = phase == 0 ? hold_ms : 200u;
            for (uint32_t ms = 0; ms < len; ms++) {
                int raw = ms < bounce_ms ? (int)(lcg() & 1u) : target;
                if (raw != prev_raw && raw == 0) r.naive_edges++; /* 沒去彈跳就數下降緣 */
                prev_raw = raw;
                int ev = debounce_step(&d, raw);
                if (ev > 0) r.debounced_press++;
                if (ev < 0) r.debounced_release++;
            }
        }
    }
    return r;
}

/* ================= 輸出與自我檢查 ================= */
static const scenario_t scenarios[] = {
    {"非阻塞主迴圈，錯過的期數會計數", POLICY_SKIP, 0, 0, 0, 10000},
    {"LED 任務 busy wait 30 ms", POLICY_SKIP, 0, 30000, 0, 10000},
    {"同 B，期限逐期補做", POLICY_CATCH_UP, 0, 30000, 0, 10000},
    {"同 B，期限從現在重新起算", POLICY_REBASE, 0, 30000, 0, 10000},
    {"同 B，改在 tick ISR 取樣", POLICY_SKIP, 1, 30000, 0, 10000},
    {"同 E，命令處理關中斷 300 us", POLICY_SKIP, 1, 30000, 300, 10000},
    {"同 E，命令處理關中斷 2500 us", POLICY_SKIP, 1, 30000, 2500, 10000},
};
#define SCENARIO_COUNT (sizeof scenarios / sizeof scenarios[0])

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
    printf("計數器回繞：從 0xFFFFFFCE 跑 100 ms，10 ms 任務應執行 10 次\n");
    int naive = count_runs_naive(0xFFFFFFCEu, 100u);
    int good = count_runs_elapsed(0xFFFFFFCEu, 100u);
    printf("  naive=%d elapsed=%d\n", naive, good);
    CHECK(good == 10);
    CHECK(naive != 10);

    printf("撕裂讀取：tick 從 0x0000FFFF 變成 0x00010000 的瞬間\n");
    g_tick32 = 0x0000FFFFu;
    g_isr_between_halves = 1;
    uint32_t torn = read_ticks_torn();
    g_tick32 = 0x0000FFFFu;
    g_isr_between_halves = 1;
    uint32_t safe = read_ticks_safe();
    printf("  torn=0x%08X safe=0x%08X\n", (unsigned)torn, (unsigned)safe);
    CHECK(torn == 0x0001FFFFu);
    CHECK(safe == 0x0000FFFFu || safe == 0x00010000u);

    stats_t r[SCENARIO_COUNT];
    for (size_t i = 0; i < SCENARIO_COUNT; i++) r[i] = run_scenario(&scenarios[i]);

    printf("A 非阻塞：每筆都準時，延遲受限於最長的單一工作\n");
    CHECK(r[0].samples == 1000u && r[0].missed_counted == 0u);
    CHECK(r[0].max_late_us < CMD_US + SAMPLE_US + 2u * TICK_ISR_US + 10u);
    printf("B busy wait：錯過的期數被計數，樣本數 + 錯過數 = 應有筆數\n");
    CHECK(r[1].samples < 1000u && r[1].samples + r[1].missed_counted == 1000u);
    CHECK(r[1].max_late_us < 1000u);       /* 做到的那幾筆仍落在格點上 */
    CHECK(r[1].max_cmd_wait_us > 25000u);  /* 命令也被卡住 */
    printf("C 逐期補做：筆數補齊，但補做的樣本晚了將近 20 ms，而且擠在一起\n");
    CHECK(r[2].samples == 1000u && r[2].max_late_us > 15000u);
    printf("D 重新起算：筆數變少，卻沒有任何計數提醒\n");
    CHECK(r[3].samples < 1000u && r[3].missed_counted == 0u);
    printf("E ISR 取樣：主迴圈阻塞不影響取樣時間\n");
    CHECK(r[4].samples == 1000u && r[4].max_late_us <= TICK_ISR_US);
    printf("F 短暫關中斷：取樣延遲上限約等於關中斷時間\n");
    CHECK(r[5].samples == 1000u && r[5].max_late_us >= 100u && r[5].max_late_us <= 300u + TICK_ISR_US);
    printf("G 關中斷超過一個 tick：tick 被合併，時間基準落後\n");
    CHECK(r[6].ticks_lost > 0u && r[6].samples < 1000u);

    printf("去彈跳：20 次按壓、每次跳動 6 ms\n");
    button_result_t b = simulate_button(20, 120u, 6u);
    printf("  沒去彈跳的下降緣=%d，去彈跳後 按下=%d 放開=%d\n", b.naive_edges, b.debounced_press,
           b.debounced_release);
    CHECK(b.naive_edges > 20);
    CHECK(b.debounced_press == 20 && b.debounced_release == 20);
    printf("去彈跳：短於門檻的 8 ms 碰觸不算按壓\n");
    button_result_t g = simulate_button(5, 8u, 0u);
    CHECK(g.debounced_press == 0);

    printf(g_fail ? "\n%d 項失敗\n" : "\n全部通過\n", g_fail);
    return g_fail ? 1 : 0;
}

int main(int argc, char **argv)
{
    console_utf8();
    if (argc > 1 && strcmp(argv[1], "--test") == 0) return run_tests();
    if (argc > 1) {
        printf("用法：sched_sim [--test]\n");
        return 2;
    }
    printf("10 ms 取樣、500 ms LED、命令隨機到達；模擬 10 秒，時間單位 us\n");
    for (size_t i = 0; i < SCENARIO_COUNT; i++)
        printf("  %c：%s\n", (char)('A' + i), scenarios[i].name);
    printf("\n%-4s %-10s %11s %6s %9s %8s %6s %9s\n", "case", "policy", "samples", "missed",
           "max_late", "avg_late", "lost", "cmd_wait");
    for (size_t i = 0; i < SCENARIO_COUNT; i++) {
        stats_t r = run_scenario(&scenarios[i]);
        print_stats((char)('A' + i), &scenarios[i], &r);
    }
    printf("\nmissed=排程器承認錯過的期數  max_late/avg_late=取樣比理想時間晚多少\n"
           "lost=關中斷期間被合併的 tick  cmd_wait=命令到達後最久等多久才被處理\n");
    printf("\n回繞：naive=%d 次，elapsed=%d 次（應為 10）\n", count_runs_naive(0xFFFFFFCEu, 100u),
           count_runs_elapsed(0xFFFFFFCEu, 100u));
    return 0;
}

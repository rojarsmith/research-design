/*
 * adc_dma.c：模擬「timer 觸發 ADC → DMA 循環寫入 → 半滿與全滿中斷 → 主迴圈批次處理」，
 * 並在每一次 CPU 讀取時檢查：這一半此刻是不是正被 DMA 寫入。
 *
 * 時間單位是微秒，全部由程式推進，每次執行結果都相同。成本數字是假設值，
 * 用來比較設計，不代表任何晶片；上板後要換成自己量到的數字。
 *
 * 硬體模型：
 *   timer 每 250 µs 觸發一次 ADC（4 kHz），轉換結果由 DMA 寫進 dma_buf[80]。
 *   buffer 分成兩半，各 40 筆，也就是 10 ms 一批。DMA 寫完前半發出 HT（half transfer），
 *   寫完後半發出 TC（transfer complete），然後回到開頭繼續寫（circular mode）。
 *   DMA 不會等 CPU；CPU 晚了，DMA 照樣寫進 CPU 還沒處理完的那一半。
 *
 * 所有權：乒乓 DMA 就是容量 2 的 SPSC 環形緩衝區（第五課 ring.c 的規則）
 *   g_head：DMA 已寫完的批次數，只有 DMA ISR 寫入
 *   g_tail：CPU 已歸還的批次數，只有主迴圈寫入
 *   批次 k 放在 slot k % 2。tail <= k < head 的批次屬於 CPU，DMA 正在寫的是 slot head % 2。
 *   head - k >= 2 表示 DMA 已經繞回批次 k 那一半：資料至少有一部分被覆寫了。
 *
 * 滿了怎麼辦（policy）：
 *   drop-old  DMA 不停。CPU 處理前後各看一次 head；被覆寫的批次整批作廢並計數。
 *   stop      ISR 發現下一半還沒歸還就停住 DMA；期間的觸發算遺失，CPU 歸還後重新啟動。
 *   none      錯誤示範：不檢查，照樣發布。數字看起來完美，內容卻可能是錯的。
 *
 * 處理方式（mode）：
 *   in-place  直接在 DMA buffer 上邊讀邊算，整個處理期間都佔著那一半
 *   copy      先把 40 筆複製出來就歸還，再處理副本
 *
 * 模型額外記錄「真相」：每一批實際是哪些取樣、CPU 每次讀取時 DMA 在寫哪一半。
 * 韌體看不到這些，測試用它們判斷韌體的回報是否誠實。
 *
 * 這個模型假設 DMA 中斷在下一筆取樣寫入前就執行完（ISR 延遲 < 250 µs）。
 * 關中斷更久時，DMA 可能已經寫進新的一半而 head 還沒更新，要改讀 DMA 的剩餘計數暫存器。
 *
 * 建置：gcc -std=c11 -Wall -Wextra -Werror -O2 -o adc_dma adc_dma.c
 * 執行：./adc_dma                 印出六個情境的統計
 *       ./adc_dma --fault=NAME    注入一種錯誤再跑一次，NAME 見 --help
 *       ./adc_dma --test          執行自我檢查
 */
#include <stdatomic.h>
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

#define SAMPLE_US 250u       /* ADC 觸發週期：4 kHz */
#define HALF 40u             /* 每一半 40 筆 = 10 ms */
#define BUF (2u * HALF)
#define RUN_BATCHES 1000u    /* 模擬 10 秒 */
#define TOTAL (RUN_BATCHES * HALF)
#define MAX_BATCHES 1100u
#define DMA_ISR_US 2u        /* DMA 中斷的成本（假設） */
#define LOOP_US 3u           /* 主迴圈沒事做時繞一圈的成本 */
#define COPY_US 10u          /* 複製 40 筆的成本（假設） */
#define DRAIN_US 50000u      /* 觸發停止後，主迴圈再跑多久把剩下的批次處理完 */

typedef enum { POL_DROP_OLD, POL_STOP, POL_NONE } policy_t;
typedef enum { MODE_IN_PLACE, MODE_COPY } proc_mode_t;
typedef enum { FAULT_NONE, FAULT_WRONG_HALF, FAULT_EARLY_RELEASE, FAULT_COUNT } fault_t;

static const char *const policy_names[] = {"drop-old", "stop", "none"};
static const char *const mode_names[] = {"in-place", "copy"};
static const char *const fault_names[FAULT_COUNT] = {"none", "wrong-half", "early-release"};

typedef struct {
    const char *name;
    policy_t policy;
    proc_mode_t mode;
    uint32_t proc_us_per_sample; /* 處理每一筆的成本 */
    uint64_t block_at_us;        /* 主迴圈在這個時刻之後卡住一次；0 表示不卡 */
    uint32_t block_us;
} scenario_t;

typedef struct {
    uint32_t first; /* 第一筆是第幾次觸發 */
    uint32_t sum;
    uint16_t min, max;
} batch_t;

typedef struct {
    uint32_t published;     /* 發布的批次數 */
    uint32_t lost_reported; /* 韌體自己回報的遺失取樣數 */
    uint32_t missing;       /* 真相：沒有出現在任何發布批次裡的取樣數 */
    uint32_t inflight;      /* 結束時還在 DMA 寫到一半的批次裡的取樣數 */
    uint32_t bad;           /* 發布了，內容卻和真實取樣不符的批次數 */
    uint32_t wrong_reads;   /* CPU 讀到 DMA 正在寫的那一半的次數 */
    uint32_t bad_owner;     /* 發布的批次裡，有讀過 DMA 正在寫的那一半 */
    uint32_t max_wait_us;   /* 批次寫完到 CPU 開始處理的最長時間 */
    uint32_t dma_irqs;
    uint32_t stops;
    batch_t batch0;         /* 發布的第 0 批 */
} result_t;

/* ---------------- 取樣來源：第 n 次觸發得到的 12 位元數值，可以隨時重算 ---------------- */
static uint16_t adc_value(uint32_t n)
{
    uint32_t h = n * 2654435761u;
    return (uint16_t)(1800u + (n % 200u) * 2u + ((h >> 24) & 0x3Fu)); /* 介於 0..4095 */
}

static batch_t reference_batch(uint32_t first)
{
    batch_t b = {first, 0u, 0xFFFFu, 0u};
    for (uint32_t i = 0; i < HALF; i++) {
        uint16_t v = adc_value(first + i);
        b.sum += v;
        if (v < b.min) b.min = v;
        if (v > b.max) b.max = v;
    }
    return b;
}

/* ---------------- 硬體：timer、ADC、DMA ---------------- */
static uint16_t dma_buf[BUF];
static struct {
    int running;
    uint32_t pos;         /* DMA 下一個要寫的位置，0..BUF-1 */
    uint32_t triggers;    /* timer 已觸發幾次，與 DMA 是否在跑無關 */
    uint32_t batch_start; /* 真相：DMA 正在寫的這一批，第一筆是第幾次觸發 */
} hw;

/* ---------------- 韌體共用狀態：每個變數只有一方寫入 ---------------- */
static _Atomic uint32_t g_head;    /* 只有 DMA ISR 寫 */
static _Atomic uint32_t g_tail;    /* 只有主迴圈寫 */
static uint32_t slot_first[2];     /* 每一半的第一筆是第幾次觸發；DMA 進入那一半時由 ISR 寫 */
static uint32_t stop_trigger;      /* stop：DMA 停住時的觸發計數；ISR 寫、DMA 停住後主迴圈讀 */
static uint32_t lost_reported;     /* 主迴圈寫 */

/* ---------------- 模擬器 ---------------- */
static const scenario_t *sc;
static fault_t g_fault;
static result_t res;
static uint64_t t_now, next_trigger;
static uint32_t truth_first[MAX_BATCHES]; /* 真相：批次 k 實際的第一筆 */
static uint64_t evt_time[MAX_BATCHES];    /* 批次 k 寫完的時刻 */
static int cur_tainted;                   /* 目前處理的這一批讀過 DMA 正在寫的那一半 */

static void dma_isr(void)
{
    res.dma_irqs++;
    uint32_t head = atomic_load_explicit(&g_head, memory_order_relaxed) + 1u;
    if (head - 1u < MAX_BATCHES) {
        truth_first[head - 1u] = hw.batch_start;
        evt_time[head - 1u] = t_now;
    }
    atomic_store_explicit(&g_head, head, memory_order_release); /* 發布：批次 head-1 交給 CPU */
    uint32_t tail = atomic_load_explicit(&g_tail, memory_order_acquire);
    if (sc->policy == POL_STOP && head - tail >= 2u) {
        hw.running = 0; /* 下一半還沒歸還：停住，不覆寫 */
        stop_trigger = hw.triggers;
        res.stops++;
        return;
    }
    slot_first[head % 2u] = hw.triggers; /* DMA 進入下一半：那一半此刻屬於 DMA，ISR 可以寫它的資訊 */
    hw.batch_start = hw.triggers;
}

/* timer 觸發一次；回傳這次讓 CPU 多花的時間（DMA ISR） */
static uint64_t adc_trigger(void)
{
    uint32_t n = hw.triggers++;
    if (!hw.running) return 0; /* 沒有人搬走：這筆轉換結果遺失（ADC 自己的 overrun） */
    dma_buf[hw.pos] = adc_value(n);
    hw.pos++;
    if (hw.pos == HALF || hw.pos == BUF) {
        if (hw.pos == BUF) hw.pos = 0;
        dma_isr();
        return DMA_ISR_US;
    }
    return 0;
}

/* 主迴圈花 us 微秒；期間到期的觸發照樣發生，DMA 中斷會把主迴圈往後推 */
static void cpu_spend(uint64_t us)
{
    uint64_t end = t_now + us;
    while (hw.triggers < TOTAL && next_trigger <= end) {
        t_now = next_trigger;
        next_trigger += SAMPLE_US;
        uint64_t c = adc_trigger();
        t_now += c;
        end += c;
    }
    t_now = end;
}

static uint16_t cpu_read(uint32_t idx)
{
    if (hw.running && idx / HALF == hw.pos / HALF) { /* DMA 正在寫這一半 */
        res.wrong_reads++;
        cur_tainted = 1;
    }
    return dma_buf[idx];
}

/* 主迴圈歸還批次；stop 策略在這裡重新啟動 DMA */
static void release_to(uint32_t new_tail)
{
    atomic_store_explicit(&g_tail, new_tail, memory_order_release);
    if (sc->policy != POL_STOP || hw.running) return;
    uint32_t head = atomic_load_explicit(&g_head, memory_order_acquire);
    if (head - new_tail >= 2u) return;
    /* DMA 停著，不會有 DMA 中斷，所以主迴圈可以安全地改這些狀態 */
    lost_reported += hw.triggers - stop_trigger;
    slot_first[head % 2u] = hw.triggers;
    hw.batch_start = hw.triggers;
    hw.pos = (head % 2u) * HALF;
    hw.running = 1;
}

static void publish(uint32_t k, const batch_t *b)
{
    res.published++;
    if (k == 0u) res.batch0 = *b;
    if (cur_tainted) res.bad_owner++;
    batch_t ref = reference_batch(k < MAX_BATCHES ? truth_first[k] : 0u);
    if (b->first != ref.first || b->sum != ref.sum || b->min != ref.min || b->max != ref.max)
        res.bad++;
}

static batch_t summarize(const uint16_t *v, uint32_t first)
{
    batch_t b = {first, 0u, 0xFFFFu, 0u};
    for (uint32_t i = 0; i < HALF; i++) {
        b.sum += v[i];
        if (v[i] < b.min) b.min = v[i];
        if (v[i] > b.max) b.max = v[i];
    }
    return b;
}

static result_t run(const scenario_t *s, fault_t fault)
{
    sc = s;
    g_fault = fault;
    memset(&res, 0, sizeof res);
    memset(&hw, 0, sizeof hw);
    memset(dma_buf, 0, sizeof dma_buf);
    atomic_store(&g_head, 0u);
    atomic_store(&g_tail, 0u);
    slot_first[0] = 0u;
    slot_first[1] = HALF;
    lost_reported = 0;
    hw.running = 1;
    t_now = 0;
    next_trigger = SAMPLE_US;
    int blocked = 0;
    uint64_t end_us = (uint64_t)TOTAL * SAMPLE_US + DRAIN_US;

    while (t_now < end_us) {
        if (s->block_at_us && !blocked && t_now >= s->block_at_us) {
            blocked = 1;
            cpu_spend(s->block_us); /* 例如等待 Flash 寫入；中斷照常發生 */
        }
        uint32_t head = atomic_load_explicit(&g_head, memory_order_acquire);
        uint32_t tail = atomic_load_explicit(&g_tail, memory_order_relaxed);
        if (head == tail) {
            cpu_spend(LOOP_US);
            continue;
        }
        /* 最舊的那一批已經等了多久：CPU 晚了多少 */
        uint32_t wait = (uint32_t)(t_now - evt_time[tail < MAX_BATCHES ? tail : 0u]);
        if (wait > res.max_wait_us) res.max_wait_us = wait;
        if (s->policy == POL_DROP_OLD && head - tail >= 2u) {
            /* 處理前就已經被繞過：只有 head-1 還完整，更舊的整批作廢並計數 */
            lost_reported += (head - 1u - tail) * HALF;
            release_to(head - 1u);
            continue;
        }

        uint32_t k = tail;
        uint32_t slot = (g_fault == FAULT_WRONG_HALF) ? (k + 1u) % 2u : k % 2u;
        cur_tainted = 0;
        uint32_t first = slot_first[slot];
        if (g_fault == FAULT_EARLY_RELEASE) release_to(k + 1u); /* 錯誤：還沒讀就歸還 */

        uint16_t v[HALF];
        if (s->mode == MODE_COPY) {
            for (uint32_t i = 0; i < HALF; i++) v[i] = cpu_read(slot * HALF + i);
            cpu_spend(COPY_US);
        } else {
            for (uint32_t i = 0; i < HALF; i++) {
                v[i] = cpu_read(slot * HALF + i);
                cpu_spend(s->proc_us_per_sample); /* 邊讀邊算：整段時間都佔著這一半 */
            }
        }

        if (s->policy == POL_DROP_OLD) {
            head = atomic_load_explicit(&g_head, memory_order_acquire);
            if (head - k >= 2u) { /* 讀的途中 DMA 已經繞回來：整批作廢 */
                lost_reported += HALF;
                if (g_fault != FAULT_EARLY_RELEASE) release_to(k + 1u);
                continue;
            }
        }
        if (g_fault != FAULT_EARLY_RELEASE) release_to(k + 1u);
        if (s->mode == MODE_COPY) cpu_spend((uint64_t)s->proc_us_per_sample * HALF); /* 處理副本 */
        batch_t b = summarize(v, first);
        publish(k, &b);
    }

    /* 結算：仍在停止中的期間也算遺失；DMA 寫到一半的批次算在途 */
    if (!hw.running) lost_reported += hw.triggers - stop_trigger;
    else res.inflight = hw.pos % HALF;
    res.lost_reported = lost_reported;
    uint32_t published_samples = res.published * HALF;
    res.missing = TOTAL - res.inflight > published_samples ? TOTAL - res.inflight - published_samples : 0u;
    return res;
}

/* ---------------- 情境 ---------------- */
static const scenario_t scenarios[] = {
    {"正常：每批處理 0.6 ms", POL_DROP_OLD, MODE_IN_PLACE, 15u, 0u, 0u},
    {"主迴圈在 3 秒時卡住 35 ms", POL_DROP_OLD, MODE_IN_PLACE, 15u, 3000000u, 35000u},
    {"同 B，滿了就停住 DMA", POL_STOP, MODE_IN_PLACE, 15u, 3000000u, 35000u},
    {"同 B，不做任何檢查（錯誤示範）", POL_NONE, MODE_IN_PLACE, 15u, 3000000u, 35000u},
    {"每批處理 12 ms，長期超過 10 ms 的預算", POL_DROP_OLD, MODE_IN_PLACE, 300u, 0u, 0u},
    {"同 E，先複製再處理", POL_DROP_OLD, MODE_COPY, 300u, 0u, 0u},
};
#define SCENARIO_COUNT (sizeof scenarios / sizeof scenarios[0])

static void print_header(void)
{
    printf("%-4s %-8s %-8s %9s %8s %8s %4s %8s %7s %8s\n", "case", "policy", "mode", "published",
           "lost_rep", "missing", "bad", "wrong_rd", "bad_own", "max_wait");
}

static void print_row(char id, const scenario_t *s, const result_t *r)
{
    printf("%-4c %-8s %-8s %9u %8u %8u %4u %8u %7u %8u\n", id, policy_names[s->policy],
           mode_names[s->mode], (unsigned)r->published, (unsigned)r->lost_reported,
           (unsigned)r->missing, (unsigned)r->bad, (unsigned)r->wrong_reads,
           (unsigned)r->bad_owner, (unsigned)r->max_wait_us);
}

static void print_table(fault_t f)
{
    printf("4 kHz 取樣、每半 40 筆（10 ms）、模擬 10 秒共 %u 批；故障：%s\n", RUN_BATCHES, fault_names[f]);
    for (size_t i = 0; i < SCENARIO_COUNT; i++)
        printf("  %c：%s\n", (char)('A' + i), scenarios[i].name);
    printf("\n");
    print_header();
    uint32_t irqs = 0;
    for (size_t i = 0; i < SCENARIO_COUNT; i++) {
        result_t r = run(&scenarios[i], f);
        if (i == 0) irqs = r.dma_irqs;
        print_row((char)('A' + i), &scenarios[i], &r);
    }
    printf("\npublished=發布的批次  lost_rep=韌體回報的遺失取樣  missing=真相：沒被發布的取樣\n"
           "bad=發布了但內容錯誤的批次  wrong_rd=CPU 讀到 DMA 正在寫的那一半的次數\n"
           "bad_own=發布的批次裡讀過 DMA 正在寫的那一半  max_wait=最舊的批次等了多久才被看到（us）\n");
    printf("\nA 的 DMA 中斷共 %u 次（每秒 %u 次）；若每筆取樣都進一次中斷，是每秒 %u 次\n", (unsigned)irqs,
           (unsigned)(irqs / 10u), (unsigned)(1000000u / SAMPLE_US));
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

/* 正確的策略：每一筆取樣不是被發布，就是被回報遺失，或還在 DMA 裡 */
static int accounted(const result_t *r)
{
    return r->published * HALF + r->lost_reported + r->inflight == TOTAL &&
           r->lost_reported == r->missing;
}

static int run_tests(void)
{
    result_t r[SCENARIO_COUNT];
    for (size_t i = 0; i < SCENARIO_COUNT; i++) r[i] = run(&scenarios[i], FAULT_NONE);

    printf("批次結果：A 發布的第 0 批 = 第 0..39 次觸發的取樣\n");
    batch_t b0 = r[0].batch0;
    printf("  first=%u sum=%u min=%u max=%u mean=%.2f\n", (unsigned)b0.first, (unsigned)b0.sum,
           (unsigned)b0.min, (unsigned)b0.max, b0.sum / (double)HALF);
    CHECK(b0.first == 0u && b0.sum == 74838u && b0.min == 1800u && b0.max == 1936u);

    printf("A 正常：1000 批全部發布且內容正確，CPU 從沒碰過 DMA 正在寫的那一半\n");
    CHECK(r[0].published == RUN_BATCHES && r[0].bad == 0u);
    CHECK(r[0].wrong_reads == 0u && r[0].lost_reported == 0u);
    CHECK(r[0].dma_irqs == 2u * TOTAL / BUF);

    printf("B 卡住 35 ms、丟舊：被繞過的 3 批整批作廢並計數，發布的內容全部正確\n");
    CHECK(r[1].bad == 0u && accounted(&r[1]));
    CHECK(r[1].lost_reported == 3u * HALF);
    CHECK(r[1].max_wait_us > 30000u);

    printf("C 卡住 35 ms、停住 DMA：沒有覆寫，停住期間的觸發全部算進遺失\n");
    CHECK(r[2].bad == 0u && accounted(&r[2]));
    CHECK(r[2].stops == 1u && r[2].lost_reported > 0u && r[2].wrong_reads == 0u);

    printf("D 不檢查：回報零遺失，發布的批次卻有錯\n");
    CHECK(r[3].lost_reported == 0u && r[3].published == RUN_BATCHES);
    CHECK(r[3].bad > 0u);

    printf("E 每批 12 ms、就地處理：每一批都在讀完前被繞過，全部作廢並計數\n");
    CHECK(r[4].bad == 0u && accounted(&r[4]));
    CHECK(r[4].published <= 1u);

    printf("F 每批 12 ms、先複製：大約每 6 批遺失 1 批，遺失全部被計數\n");
    CHECK(r[5].bad == 0u && accounted(&r[5]) && r[5].wrong_reads == 0u);
    CHECK(r[5].published > 800u && r[5].published < 870u);

    printf("故障 wrong-half：HT 時處理了 DMA 正在寫的那一半\n");
    result_t w = run(&scenarios[0], FAULT_WRONG_HALF);
    printf("  wrong_rd=%u bad=%u\n", (unsigned)w.wrong_reads, (unsigned)w.bad);
    CHECK(w.wrong_reads > 0u && w.bad_owner > 0u);
    CHECK(w.bad > 0u);

    printf("故障 early-release：還沒讀就歸還，停住的 DMA 立刻寫進 CPU 正在讀的那一半\n");
    result_t e = run(&scenarios[2], FAULT_EARLY_RELEASE);
    printf("  wrong_rd=%u bad_own=%u bad=%u\n", (unsigned)e.wrong_reads, (unsigned)e.bad_owner,
           (unsigned)e.bad);
    if (e.bad == 0u) printf("  內容碰巧還沒被蓋到；比對內容抓不到，所有權檢查抓得到\n");
    CHECK(e.wrong_reads > 0u && e.bad_owner > 0u);

    printf(g_fail ? "\n%d 項失敗\n" : "\n全部通過\n", g_fail);
    return g_fail ? 1 : 0;
}

static void usage(void)
{
    printf("用法：adc_dma [--test] [--fault=NAME]\n故障名稱：");
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
    print_table(f);
    return 0;
}

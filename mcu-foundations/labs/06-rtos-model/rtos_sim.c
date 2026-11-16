/*
 * rtos_sim.c：用虛擬時間模擬一個很小的優先權搶占式 kernel，把資料採集器拆成幾個 task 執行，
 * 再和超級迴圈版本比較延遲與記憶體。
 *
 * 時間單位是微秒，全部由程式推進，每次執行結果都相同。所有成本數字都是假設值，
 * 用來比較設計，不代表任何晶片或 RTOS；上板後要換成自己量到的數字。
 *
 * kernel 模型（概念對應 FreeRTOS 或 Zephyr，API 名稱是自訂的）：
 *   tick 每 1 ms 一次；ready 的 task 中，有效優先權最高的那個執行（數字越大越優先，
 *   這是 FreeRTOS 的慣例；Zephyr 相反，數字越小越優先）。
 *   queue：固定長度，項目以複製方式傳遞。task 用 q_recv 可以阻塞等待；
 *     ISR 只能用 q_send_from_isr，滿了立刻失敗，不會等。
 *   mutex：只能由 task 使用，可選擇優先權繼承；unlock 時直接交給等待者中優先權最高的。
 *   ISR 喚醒較高優先權的 task 時，要在結尾要求切換；沒要求的話，等到下一個 tick 才切換。
 *   在 ISR 裡呼叫 task 用的 API（可能阻塞的 q_send、mutex、delay）會被記錄成誤用，呼叫失敗。
 *
 * 每個 task 寫成一步一步的狀態機：每一步是不可分割的動作，步與步之間、以及計算途中，
 * 都可能被中斷與較高優先權的 task 搶占。這就是「可被打斷的位置」。
 *
 * 資料採集器的工作：
 *   sample  DMA 每 10 ms 送來一批（第六課）。讀那一半、算平均 300 µs，
 *           必須在 DMA 繞回來之前讀完，否則整批作廢並計數；然後把取樣封包寫進 TX。
 *   cmd     收 UART 位元組，交給第五課的 frame.c 解析；收到 GET_STATUS 就回 STATUS。
 *   stats   每 100 ms 做一次 25 ms 的統計運算，只用 CPU。
 *   diag    每 100 ms 送一個 STATUS 自我回報；收集資料 1 ms，每個 byte 格式化 100 µs。
 *   TX      所有封包寫進同一個 1024-byte 的 TX 環形緩衝區，由 TX DMA 依線路速度送出。
 *
 * 只要有兩個 task 都寫 TX，第五課 SPSC 的前提「只有一個生產者」就不成立了。
 *
 * 建置（一行）：
 *   gcc -std=c11 -Wall -Wextra -Werror -O2 -I../04-uart-pipeline -o rtos_sim rtos_sim.c ../04-uart-pipeline/frame.c
 * 執行：./rtos_sim                 印出六種設計的比較、記憶體與各 task 的統計
 *       ./rtos_sim --fault=NAME    注入一種錯誤再跑一次，NAME 見 --help
 *       ./rtos_sim --test          執行自我檢查
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "frame.h"

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

#define NEVER UINT64_MAX
#define RUN_US 10000000ull     /* 模擬 10 秒 */
#define TICK_US 1000u
#define BATCH_US 10000u        /* DMA 每 10 ms 一批 */
#define BATCH_PHASE_US 250u    /* DMA 與 tick 用不同的時脈，事件不會剛好對齊 */
#define BYTE_US 87u            /* 115200 baud、8N1 */
#define TX_CAP 1024u

/* 成本（假設值） */
#define CS_US 4u               /* 一次 context switch */
#define ISR_US 2u              /* 一次中斷的進出與本體 */
#define STEP_US 1u             /* task 每一步至少花的時間 */
#define SAMPLE_PROC_US 300u
#define BYTE_COPY_US 2u        /* 把一個已經準備好的 byte 寫進 TX */
#define CMD_BYTE_US 5u
#define CMD_US 50u
#define DIAG_GATHER_US 1000u
#define DIAG_FMT_BYTE_US 100u
#define STATS_US 25000u
#define STATS_CHUNK_US 1000u
#define LOOP_US 3u
#define DIAG_PHASE_US 9000u
#define STATS_PHASE_US 11000u
#define PERIOD_100MS 100000u

typedef enum { FAULT_NONE, FAULT_ISR_BLOCKING, FAULT_NO_YIELD, FAULT_COUNT } fault_t;
static const char *const fault_names[FAULT_COUNT] = {"none", "isr-blocking", "no-yield"};

typedef struct {
    const char *name;
    int rtos;              /* 0：超級迴圈 */
    int chunk_stats;       /* 超級迴圈：stats 切成 1 ms 小段 */
    int use_mutex, inherit;
    int diag_slow_in_lock; /* 1：diag 邊寫 TX 邊格式化，而且一開始就上鎖 */
} config_t;

/* ================= kernel ================= */
enum { T_READY, T_BLOCKED };
enum { W_NONE, W_DELAY, W_RECV, W_MUTEX };
enum { K_OK, K_EMPTY, K_BLOCKED, K_ERR };

typedef struct task task_t;

typedef struct {
    const char *name;
    uint8_t *mem;
    uint32_t item, len, rd, count;
    uint32_t hwm, full_drops; /* 最高佔用、ISR 送不進去的次數 */
} queue_t;

typedef struct {
    task_t *owner;
    int inherit;
} mutex_t;

typedef struct {
    int pc;
    uint32_t k;          /* 批次號 */
    uint64_t t_evt;      /* 批次寫完的時刻 */
    uint8_t buf[FRAME_MAX_SIZE];
    uint32_t n, byte_us;
    int wstate;
    uint32_t w, wi;      /* TX：讀到的 head、寫到第幾個 byte */
    uint32_t remaining;  /* stats 還剩多少運算 */
} job_t;

struct task {
    const char *name;
    int base_prio, prio, state, pc;
    uint32_t compute;    /* 這一步還剩多少可被搶占的運算時間 */
    int waiting, wait_kind;
    void *wait_obj;
    uint64_t wake_at, ready_at, block_start, next;
    int delay_init;
    void (*body)(task_t *);
    uint32_t stack_bytes; /* 設定值，不是量到的用量 */
    job_t job;
    uint64_t cpu_us;
    uint32_t switches_in, preempted, max_ready_wait, max_block;
};

static struct {
    uint64_t now;
    int in_isr, switch_req, yield_pending;
    task_t *cur;
    task_t *tasks[8];
    int ntasks;
    uint32_t ticks, switches, isrs, misuse;
    uint64_t kernel_us, isr_us;
} k;

static const config_t *cfg;
static fault_t g_fault;

static void make_ready(task_t *t)
{
    t->state = T_READY;
    t->wait_kind = W_NONE;
    t->ready_at = k.now;
}

static void block(task_t *t, int kind, void *obj, uint64_t wake_at)
{
    t->state = T_BLOCKED;
    t->waiting = 1;
    t->wait_kind = kind;
    t->wait_obj = obj;
    t->wake_at = wake_at;
    t->block_start = k.now;
}

static task_t *highest_waiter(int kind, void *obj)
{
    task_t *best = NULL;
    for (int i = 0; i < k.ntasks; i++) {
        task_t *t = k.tasks[i];
        if (t->state == T_BLOCKED && t->wait_kind == kind && t->wait_obj == obj &&
            (!best || t->prio > best->prio))
            best = t;
    }
    return best;
}

static int q_put(queue_t *q, const void *item)
{
    if (q->count == q->len) {
        q->full_drops++;
        return 0;
    }
    memcpy(q->mem + ((q->rd + q->count) % q->len) * q->item, item, q->item);
    q->count++;
    if (q->count > q->hwm) q->hwm = q->count;
    return 1;
}

/* task 用：forever 為 1 時沒有資料就阻塞 */
static int q_recv(task_t *t, queue_t *q, void *item, int forever)
{
    if (k.in_isr) {
        k.misuse++;
        return K_ERR;
    }
    if (q->count) {
        memcpy(item, q->mem + q->rd * q->item, q->item);
        q->rd = (q->rd + 1u) % q->len;
        q->count--;
        t->waiting = 0;
        return K_OK;
    }
    if (!forever) return K_EMPTY;
    block(t, W_RECV, q, NEVER);
    return K_BLOCKED;
}

/* task 用的送出：可能阻塞，所以不能在 ISR 裡呼叫 */
static int q_send(queue_t *q, const void *item)
{
    if (k.in_isr) { /* 相當於 FreeRTOS 的 configASSERT：記錄並拒絕 */
        k.misuse++;
        return K_ERR;
    }
    if (!q_put(q, item)) return K_ERR;
    task_t *w = highest_waiter(W_RECV, q);
    if (w) {
        make_ready(w);
        if (w->prio > k.cur->prio) k.switch_req = 1;
    }
    return K_OK;
}

/* ISR 用：不阻塞；喚醒了比目前 task 更高優先權的 task 時，設定 *woken */
static int q_send_from_isr(queue_t *q, const void *item, int *woken)
{
    if (!q_put(q, item)) return 0;
    task_t *w = highest_waiter(W_RECV, q);
    if (w) {
        make_ready(w);
        if (!k.cur || w->prio > k.cur->prio) {
            *woken = 1;
            k.yield_pending = 1; /* 就算 ISR 沒要求切換，下一個 tick 也會切 */
        }
    }
    return 1;
}

static void isr_exit(int woken)
{
    if (woken && g_fault != FAULT_NO_YIELD) k.switch_req = 1; /* 相當於 portYIELD_FROM_ISR */
}

static int mtx_lock(task_t *t, mutex_t *m)
{
    if (k.in_isr) {
        k.misuse++;
        return 0;
    }
    if (m->owner == t) {
        if (t->waiting) { /* 被 unlock 直接交到手上 */
            t->waiting = 0;
            uint32_t b = (uint32_t)(k.now - t->block_start);
            if (b > t->max_block) t->max_block = b;
        }
        return 1;
    }
    if (!m->owner) {
        m->owner = t;
        return 1;
    }
    block(t, W_MUTEX, m, NEVER);
    if (m->inherit && m->owner->prio < t->prio) m->owner->prio = t->prio; /* 優先權繼承 */
    return 0;
}

static void mtx_unlock(task_t *t, mutex_t *m)
{
    if (k.in_isr) {
        k.misuse++;
        return;
    }
    m->owner = NULL;
    t->prio = t->base_prio; /* 本模型每個 task 同時最多持有一個 mutex */
    task_t *w = highest_waiter(W_MUTEX, m);
    if (w) {
        m->owner = w;
        make_ready(w);
    }
    k.switch_req = 1;
}

/* 固定週期：第一次在 phase，之後每 period 一次，由 tick 喚醒 */
static int k_delay_until(task_t *t, uint64_t phase, uint64_t period)
{
    if (k.in_isr) {
        k.misuse++;
        return 0;
    }
    if (t->waiting) {
        t->waiting = 0;
        return 1;
    }
    t->next = t->delay_init ? t->next + period : phase;
    t->delay_init = 1;
    if (t->next <= k.now) return 1;
    block(t, W_DELAY, NULL, t->next);
    return 0;
}

static task_t *pick(void)
{
    task_t *best = NULL;
    for (int i = 0; i < k.ntasks; i++) {
        task_t *t = k.tasks[i];
        if (t->state == T_READY && (!best || t->prio > best->prio)) best = t;
    }
    return best;
}

/* ================= 資料採集器的共用資源 ================= */
static uint8_t batch_mem[4 * sizeof(uint32_t)];
static uint8_t rx_mem[32];
static queue_t batch_q = {"batch_q", batch_mem, sizeof(uint32_t), 4u, 0, 0, 0, 0};
static queue_t rx_q = {"rx_q", rx_mem, 1u, 32u, 0, 0, 0, 0};
static mutex_t tx_mtx;
static struct {
    uint8_t buf[TX_CAP];
    uint32_t head, tail; /* head：由寫 TX 的 task 發布；tail：TX DMA 送出 */
} txr;
static parser_t cmd_parser;
static int cmd_ready;

#define MAX_CMDS 600u
static uint64_t rx_time[MAX_CMDS * 5u];
static uint8_t rx_byte[MAX_CMDS * 5u];
static uint32_t rx_n, rx_i;
static uint64_t cmd_last_byte[MAX_CMDS];
static uint32_t cmd_n;

static struct {
    uint32_t samples_sent, status_sent, lost_lap, lost_qfull, tx_drops, rx_lost, responses;
    uint32_t max_sample_lat, max_cmd_lat;
} dev;

static struct {
    parser_t parser;
    int have_seq;
    uint16_t last_seq;
    uint32_t samples, gaps, statuses;
} host;

static void host_on_frame(void *ctx, const frame_t *f)
{
    (void)ctx;
    if (f->type == TYPE_SAMPLE && f->len == SAMPLE_PAYLOAD) {
        uint16_t seq = get_u16(f->payload);
        if (host.have_seq && (uint16_t)(seq - host.last_seq) != 1u)
            host.gaps += (uint16_t)(seq - host.last_seq - 1u);
        host.have_seq = 1;
        host.last_seq = seq;
        host.samples++;
    } else if (f->type == TYPE_STATUS) {
        host.statuses++;
    }
}

static void on_cmd(void *ctx, const frame_t *f)
{
    (void)ctx;
    if (f->type == TYPE_GET_STATUS) cmd_ready = 1;
}

/* TX：讀 head、逐 byte 寫入、發布 head。回傳 0 還沒寫完、1 已發布、2 空間不夠而放棄 */
static int tx_step(task_t *t, job_t *j)
{
    if (j->wstate == 0) {
        j->w = txr.head; /* 從這裡到發布之前被打斷，另一個寫入者會讀到同一個 head */
        int32_t used = (int32_t)(j->w - txr.tail);
        if (used < 0) used = 0;
        if (TX_CAP - (uint32_t)used < j->n) {
            dev.tx_drops++;
            return 2;
        }
        j->wstate = 1;
        j->wi = 0;
        return 0;
    }
    if (j->wi < j->n) {
        txr.buf[(j->w + j->wi) % TX_CAP] = j->buf[j->wi];
        j->wi++;
        t->compute += j->byte_us;
        return 0;
    }
    txr.head = j->w + j->n; /* 發布 */
    j->wstate = 0;
    return 1;
}

static uint64_t batch_time(uint32_t kk) { return (uint64_t)(kk + 1u) * BATCH_US + BATCH_PHASE_US; }

static void build_status(job_t *j)
{
    uint8_t p[10];
    put_u16(&p[0], 10u);
    put_u16(&p[2], (uint16_t)(dev.lost_lap + dev.lost_qfull));
    put_u16(&p[4], (uint16_t)(parser_errors(&cmd_parser) + rx_q.full_drops));
    put_u16(&p[6], 0u);
    put_u16(&p[8], 0u);
    j->n = frame_encode(TYPE_STATUS, p, sizeof p, j->buf);
}

/* ---- 各項工作：超級迴圈與 RTOS 共用同一份 ---- */
static int sample_step(task_t *t, job_t *j)
{
    switch (j->pc) {
    case 0:
        t->compute += SAMPLE_PROC_US; /* 讀 DMA 的那一半並計算平均 */
        j->pc = 1;
        return 0;
    case 1: {
        if (k.now >= j->t_evt + BATCH_US) { /* 讀完時 DMA 已經繞回這一半：作廢並計數 */
            dev.lost_lap++;
            return 1;
        }
        uint8_t p[SAMPLE_PAYLOAD];
        put_u16(&p[0], (uint16_t)j->k);
        put_u16(&p[2], (uint16_t)(j->t_evt / 1000u));
        put_u16(&p[4], (uint16_t)(1870u + j->k % 50u));
        p[6] = 0u;
        j->n = frame_encode(TYPE_SAMPLE, p, SAMPLE_PAYLOAD, j->buf);
        j->byte_us = BYTE_COPY_US;
        j->pc = 2;
        return 0;
    }
    case 2:
        if (cfg->use_mutex && !mtx_lock(t, &tx_mtx)) return 0;
        j->pc = 3;
        return 0;
    default: {
        int r = tx_step(t, j);
        if (r == 0) return 0;
        if (r == 1) {
            dev.samples_sent++;
            uint32_t lat = (uint32_t)(k.now - j->t_evt);
            if (lat > dev.max_sample_lat) dev.max_sample_lat = lat;
        }
        if (cfg->use_mutex) mtx_unlock(t, &tx_mtx);
        return 1;
    }
    }
}

static int cmd_step(task_t *t, job_t *j)
{
    switch (j->pc) {
    case 0:
        t->compute += CMD_US;
        build_status(j);
        j->byte_us = BYTE_COPY_US;
        j->pc = 1;
        return 0;
    case 1:
        if (cfg->use_mutex && !mtx_lock(t, &tx_mtx)) return 0;
        j->pc = 2;
        return 0;
    default: {
        int r = tx_step(t, j);
        if (r == 0) return 0;
        if (r == 1) {
            dev.status_sent++;
            if (dev.responses < cmd_n) {
                uint32_t lat = (uint32_t)(k.now - cmd_last_byte[dev.responses]);
                if (lat > dev.max_cmd_lat) dev.max_cmd_lat = lat;
            }
        }
        dev.responses++;
        if (cfg->use_mutex) mtx_unlock(t, &tx_mtx);
        return 1;
    }
    }
}

static int diag_step(task_t *t, job_t *j)
{
    switch (j->pc) {
    case 0: /* 慢的寫法：先上鎖，再在鎖裡收集與格式化 */
        if (!cfg->diag_slow_in_lock) {
            t->compute += DIAG_GATHER_US + 15u * DIAG_FMT_BYTE_US; /* 快的寫法：鎖外先準備好 */
            j->pc = 10;
            return 0;
        }
        if (cfg->use_mutex && !mtx_lock(t, &tx_mtx)) return 0;
        t->compute += DIAG_GATHER_US;
        j->pc = 1;
        return 0;
    case 1:
        build_status(j);
        j->byte_us = DIAG_FMT_BYTE_US; /* 邊寫 TX 邊格式化 */
        j->pc = 20;
        return 0;
    case 10:
        build_status(j);
        j->byte_us = BYTE_COPY_US;
        j->pc = 11;
        return 0;
    case 11:
        if (cfg->use_mutex && !mtx_lock(t, &tx_mtx)) return 0;
        j->pc = 20;
        return 0;
    default: {
        int r = tx_step(t, j);
        if (r == 0) return 0;
        if (r == 1) dev.status_sent++;
        if (cfg->use_mutex) mtx_unlock(t, &tx_mtx);
        return 1;
    }
    }
}

static int stats_step(task_t *t, job_t *j)
{
    if (j->remaining == 0u) j->remaining = STATS_US;
    uint32_t c = cfg->chunk_stats && j->remaining > STATS_CHUNK_US ? STATS_CHUNK_US : j->remaining;
    t->compute += c;
    j->remaining -= c;
    return j->remaining == 0u;
}

/* ---- RTOS 版本：四個 task 加 idle ---- */
static void sample_body(task_t *t)
{
    job_t *j = &t->job;
    if (t->pc == 0) {
        if (q_recv(t, &batch_q, &j->k, 1) != K_OK) return;
        j->t_evt = batch_time(j->k);
        j->pc = 0;
        t->pc = 1;
        return;
    }
    if (sample_step(t, j)) t->pc = 0;
}

static void cmd_body(task_t *t)
{
    job_t *j = &t->job;
    if (t->pc == 0) {
        uint8_t b;
        if (q_recv(t, &rx_q, &b, 1) != K_OK) return;
        t->compute += CMD_BYTE_US;
        parser_feed(&cmd_parser, b, (uint32_t)(k.now / 1000u));
        if (cmd_ready) {
            cmd_ready = 0;
            j->pc = 0;
            t->pc = 1;
        }
        return;
    }
    if (cmd_step(t, j)) t->pc = 0;
}

static void diag_body(task_t *t)
{
    if (t->pc == 0) {
        if (!k_delay_until(t, DIAG_PHASE_US, PERIOD_100MS)) return;
        t->job.pc = 0;
        t->pc = 1;
        return;
    }
    if (diag_step(t, &t->job)) t->pc = 0;
}

static void stats_body(task_t *t)
{
    if (t->pc == 0) {
        if (!k_delay_until(t, STATS_PHASE_US, PERIOD_100MS)) return;
        t->job.remaining = 0u;
        t->pc = 1;
        return;
    }
    if (stats_step(t, &t->job)) t->pc = 0;
}

static void idle_body(task_t *t) { t->compute += 1000u; }

/* ---- 超級迴圈版本：一個無窮迴圈依序檢查每件事 ---- */
static struct {
    int (*active)(task_t *, job_t *);
    job_t job, stats_job;
    uint64_t next_diag, next_stats;
    int stats_pending;
} L;

static void loop_body(task_t *t)
{
    if (L.active) {
        if (L.active(t, &L.job)) L.active = NULL;
        return;
    }
    uint32_t kk;
    if (q_recv(t, &batch_q, &kk, 0) == K_OK) {
        memset(&L.job, 0, sizeof L.job);
        L.job.k = kk;
        L.job.t_evt = batch_time(kk);
        L.active = sample_step;
        return;
    }
    uint8_t b;
    if (q_recv(t, &rx_q, &b, 0) == K_OK) {
        t->compute += CMD_BYTE_US;
        parser_feed(&cmd_parser, b, (uint32_t)(k.now / 1000u));
        if (cmd_ready) {
            cmd_ready = 0;
            memset(&L.job, 0, sizeof L.job);
            L.active = cmd_step;
        }
        return;
    }
    if (k.now >= L.next_diag) {
        L.next_diag += PERIOD_100MS;
        memset(&L.job, 0, sizeof L.job);
        L.active = diag_step;
        return;
    }
    if (k.now >= L.next_stats) {
        L.next_stats += PERIOD_100MS;
        L.stats_pending = 1;
        L.stats_job.remaining = 0u;
    }
    if (L.stats_pending) {
        if (stats_step(t, &L.stats_job)) L.stats_pending = 0;
        return;
    }
    t->compute += LOOP_US;
}

/* ================= 任務表 ================= */
static task_t t_sample = {.name = "sample", .base_prio = 4, .body = sample_body, .stack_bytes = 512};
static task_t t_cmd = {.name = "cmd", .base_prio = 3, .body = cmd_body, .stack_bytes = 768};
static task_t t_stats = {.name = "stats", .base_prio = 2, .body = stats_body, .stack_bytes = 512};
static task_t t_diag = {.name = "diag", .base_prio = 1, .body = diag_body, .stack_bytes = 512};
static task_t t_idle = {.name = "idle", .base_prio = 0, .body = idle_body, .stack_bytes = 256};
static task_t t_loop = {.name = "main", .base_prio = 1, .body = loop_body, .stack_bytes = 1024};
#define ISR_STACK_BYTES 512u /* RTOS：中斷用另一個 stack（Cortex-M 的 MSP） */

static void reset_task(task_t *t)
{
    task_t keep = *t;
    memset(t, 0, sizeof *t);
    t->name = keep.name;
    t->base_prio = t->prio = keep.base_prio;
    t->body = keep.body;
    t->stack_bytes = keep.stack_bytes;
    t->state = T_READY;
}

/* ================= 中斷與硬體事件 ================= */
static uint64_t next_tick, next_batch, next_tx;
static uint32_t batch_counter;

static void tick_isr(void)
{
    k.ticks++;
    for (int i = 0; i < k.ntasks; i++) {
        task_t *t = k.tasks[i];
        if (t->state == T_BLOCKED && t->wait_kind == W_DELAY && t->wake_at <= k.now) {
            make_ready(t);
            if (t->prio > k.cur->prio) k.switch_req = 1;
        }
    }
    if (k.yield_pending) {
        k.switch_req = 1;
        k.yield_pending = 0;
    }
}

static void batch_isr(void)
{
    uint32_t kk = batch_counter++;
    int woken = 0;
    if (!q_send_from_isr(&batch_q, &kk, &woken)) dev.lost_qfull++; /* 滿了：不等，計數 */
    isr_exit(woken);
}

static void rx_isr(uint8_t b)
{
    int woken = 0;
    if (g_fault == FAULT_ISR_BLOCKING) {
        if (q_send(&rx_q, &b) != K_OK) dev.rx_lost++; /* 錯誤：在 ISR 裡用了 task 的 API */
    } else if (!q_send_from_isr(&rx_q, &b, &woken)) {
        dev.rx_lost++;
    }
    isr_exit(woken);
}

static uint64_t next_event(void)
{
    uint64_t t = next_tick;
    if (next_batch < t) t = next_batch;
    if (rx_i < rx_n && rx_time[rx_i] < t) t = rx_time[rx_i];
    if (next_tx < t) t = next_tx;
    return t;
}

static void fire_event(void)
{
    uint64_t t = next_event();
    if (t == next_tx) { /* TX DMA：不花 CPU */
        next_tx += BYTE_US;
        if ((int32_t)(txr.head - txr.tail) > 0) {
            uint8_t b = txr.buf[txr.tail % TX_CAP];
            txr.tail++;
            parser_feed(&host.parser, b, (uint32_t)(k.now / 1000u));
        }
        return;
    }
    k.in_isr = 1;
    if (t == next_tick) {
        next_tick += TICK_US;
        tick_isr();
    } else if (t == next_batch) {
        next_batch += BATCH_US;
        batch_isr();
    } else {
        rx_isr(rx_byte[rx_i++]);
    }
    k.in_isr = 0;
    k.isrs++;
    k.now += ISR_US;
    k.isr_us += ISR_US;
}

static int schedule(void)
{
    k.switch_req = 0;
    task_t *n = pick();
    if (!n || n == k.cur) return 0;
    if (k.cur && k.cur->state == T_READY) {
        k.cur->preempted++;
        k.cur->ready_at = k.now;
    }
    k.switches++;
    k.now += CS_US;
    k.kernel_us += CS_US;
    k.cur = n;
    n->switches_in++;
    uint32_t w = (uint32_t)(k.now - n->ready_at);
    if (w > n->max_ready_wait) n->max_ready_wait = w;
    return 1;
}

static void simulate(uint64_t end)
{
    while (k.now < end) {
        uint64_t tn = next_event();
        if (tn <= k.now) {
            fire_event();
            continue;
        }
        if (!k.cur || k.cur->state != T_READY || k.switch_req) {
            if (schedule()) continue;
        }
        task_t *t = k.cur;
        if (t->compute) {
            uint64_t dt = t->compute;
            if (tn - k.now < dt) dt = tn - k.now;
            t->compute -= (uint32_t)dt;
            t->cpu_us += dt;
            k.now += dt;
            continue;
        }
        t->body(t);
        if (t->state == T_READY) {
            t->compute += STEP_US;
        } else {
            k.now += STEP_US;
            k.kernel_us += STEP_US;
        }
    }
}

/* 電腦每隔 20 到 60 ms 送一個 GET_STATUS（5 bytes） */
static void make_commands(void)
{
    uint32_t lcg = 77u;
    uint64_t t = 3700u;
    rx_n = cmd_n = 0;
    uint8_t buf[FRAME_MAX_SIZE];
    uint32_t n = frame_encode(TYPE_GET_STATUS, NULL, 0u, buf);
    while (t < RUN_US - 100000u && cmd_n < MAX_CMDS) {
        for (uint32_t i = 0; i < n; i++) {
            t += BYTE_US;
            rx_time[rx_n] = t;
            rx_byte[rx_n++] = buf[i];
        }
        cmd_last_byte[cmd_n++] = t;
        lcg = lcg * 1664525u + 1013904223u;
        t += 20000u + (lcg >> 8) % 40000u;
    }
}

typedef struct {
    uint32_t samples_sent, host_samples, host_gaps, lost_reported, crc_err, status_sent, host_statuses;
    uint32_t max_sample_lat, max_cmd_lat, max_block, switches, misuse, responses, commands;
    uint64_t kernel_us, isr_us, idle_us;
    uint32_t batch_hwm, rx_hwm;
} result_t;

static result_t run(const config_t *c, fault_t f)
{
    cfg = c;
    g_fault = f;
    memset(&k, 0, sizeof k);
    memset(&dev, 0, sizeof dev);
    memset(&host, 0, sizeof host);
    memset(&txr, 0, sizeof txr);
    memset(&L, 0, sizeof L);
    batch_q.rd = batch_q.count = batch_q.hwm = batch_q.full_drops = 0;
    rx_q.rd = rx_q.count = rx_q.hwm = rx_q.full_drops = 0;
    tx_mtx.owner = NULL;
    tx_mtx.inherit = c->inherit;
    parser_init(&cmd_parser, 20u, on_cmd, NULL);
    parser_init(&host.parser, 1000u, host_on_frame, NULL);
    cmd_ready = 0;
    make_commands();
    rx_i = 0;
    batch_counter = 0;
    next_tick = TICK_US;
    next_batch = batch_time(0u);
    next_tx = BYTE_US;

    if (c->rtos) {
        task_t *list[] = {&t_sample, &t_cmd, &t_stats, &t_diag, &t_idle};
        k.ntasks = 5;
        for (int i = 0; i < k.ntasks; i++) {
            reset_task(list[i]);
            k.tasks[i] = list[i];
        }
    } else {
        reset_task(&t_loop);
        k.tasks[0] = &t_loop;
        k.ntasks = 1;
        L.next_diag = DIAG_PHASE_US;
        L.next_stats = STATS_PHASE_US;
    }

    simulate(RUN_US);

    result_t r;
    memset(&r, 0, sizeof r);
    r.samples_sent = dev.samples_sent;
    r.host_samples = host.samples;
    r.host_gaps = host.gaps;
    r.lost_reported = dev.lost_lap + dev.lost_qfull;
    r.crc_err = host.parser.err_crc;
    r.status_sent = dev.status_sent;
    r.host_statuses = host.statuses;
    r.max_sample_lat = dev.max_sample_lat;
    r.max_cmd_lat = dev.max_cmd_lat;
    for (int i = 0; i < k.ntasks; i++)
        if (k.tasks[i]->max_block > r.max_block) r.max_block = k.tasks[i]->max_block;
    r.switches = k.switches;
    r.misuse = k.misuse;
    r.responses = dev.responses;
    r.commands = cmd_n;
    r.kernel_us = k.kernel_us;
    r.isr_us = k.isr_us;
    r.idle_us = c->rtos ? t_idle.cpu_us : 0u;
    r.batch_hwm = batch_q.hwm;
    r.rx_hwm = rx_q.hwm;
    return r;
}

/* ================= 輸出 ================= */
static const config_t configs[] = {
    {"超級迴圈，stats 一次做完 25 ms", 0, 0, 0, 0, 1},
    {"超級迴圈，stats 切成 1 ms 小段", 0, 1, 0, 0, 1},
    {"RTOS，寫 TX 沒有 mutex", 1, 0, 0, 0, 1},
    {"RTOS，mutex 沒有優先權繼承", 1, 0, 1, 0, 1},
    {"RTOS，mutex 加優先權繼承", 1, 0, 1, 1, 1},
    {"同 E，diag 在鎖外先格式化，鎖裡只搬 15 bytes", 1, 0, 1, 1, 0},
};
#define CONFIG_COUNT (sizeof configs / sizeof configs[0])

static void print_tasks(void)
{
    printf("%-7s %4s %6s %8s %6s %9s %9s %10s\n", "task", "prio", "stack", "cpu_ms", "in", "preempted",
           "max_wait", "max_block");
    for (int i = 0; i < k.ntasks; i++) {
        task_t *t = k.tasks[i];
        printf("%-7s %4d %6u %8.1f %6u %9u %9u %10u\n", t->name, t->base_prio, (unsigned)t->stack_bytes,
               t->cpu_us / 1000.0, (unsigned)t->switches_in, (unsigned)t->preempted,
               (unsigned)t->max_ready_wait, (unsigned)t->max_block);
    }
}

static uint32_t stack_total(int rtos)
{
    if (!rtos) return t_loop.stack_bytes; /* 中斷與 main 共用同一個 stack */
    return t_sample.stack_bytes + t_cmd.stack_bytes + t_stats.stack_bytes + t_diag.stack_bytes +
           t_idle.stack_bytes + ISR_STACK_BYTES;
}

static void print_memory(void)
{
    uint32_t qbytes = batch_q.item * batch_q.len + rx_q.item * rx_q.len;
    uint32_t bufs = TX_CAP + 160u; /* TX 環形緩衝區 + 第六課的 DMA buffer（80 筆 uint16_t） */
    printf("%8s %8s  RAM（bytes）\n", "loop", "rtos");
    printf("%8u %8u  stack（設定值，不是量到的用量）\n", (unsigned)stack_total(0), (unsigned)stack_total(1));
    printf("%8u %8u  queue 內容（項目大小 x 長度）\n", (unsigned)qbytes, (unsigned)qbytes);
    printf("%8u %8u  TX 與 DMA buffer\n", (unsigned)bufs, (unsigned)bufs);
    printf("%8s %8s  TCB、queue 與 mutex 的控制區塊\n", "0", "?");
    printf("模型只算得出前三列。控制區塊大小依 RTOS 與設定而定，例如 FreeRTOS 的 StaticTask_t、\n"
           "Zephyr 的 struct k_thread，要在目標平台上用 sizeof 或 map file 查。\n");
}

static void print_all(fault_t f)
{
    printf("資料採集器：sample 每 10 ms、stats 與 diag 每 100 ms、命令隨機到達；模擬 10 秒；故障：%s\n", fault_names[f]);
    for (size_t i = 0; i < CONFIG_COUNT; i++) printf("  %c：%s\n", (char)('A' + i), configs[i].name);
    printf("\n%-4s %6s %6s %5s %5s %4s %7s %10s %10s %9s %8s\n", "case", "smp_tx", "smp_rx", "gaps", "lost",
           "crc", "st_tx/rx", "smp_lat", "cmd_lat", "max_block", "switches");
    result_t rows[CONFIG_COUNT];
    for (size_t i = 0; i < CONFIG_COUNT; i++) {
        result_t r = run(&configs[i], configs[i].rtos ? f : FAULT_NONE);
        rows[i] = r;
        char st[16];
        snprintf(st, sizeof st, "%u/%u", (unsigned)r.status_sent, (unsigned)r.host_statuses);
        printf("%-4c %6u %6u %5u %5u %4u %8s %10u %10u %9u %8u\n", (char)('A' + i), (unsigned)r.samples_sent,
               (unsigned)r.host_samples, (unsigned)r.host_gaps, (unsigned)r.lost_reported, (unsigned)r.crc_err,
               st, (unsigned)r.max_sample_lat, (unsigned)r.max_cmd_lat, (unsigned)r.max_block,
               (unsigned)r.switches);
    }
    printf("\nsmp_tx=裝置送出的取樣封包  smp_rx=電腦收到的  gaps=電腦看到的序號跳號  lost=裝置自己回報的遺失\n"
           "crc=電腦端 CRC 錯誤  st_tx/rx=STATUS 封包送出與收到  smp_lat=批次寫完到封包進 TX（us）\n"
           "cmd_lat=命令最後一個 byte 到回應進 TX（us）  max_block=等 mutex 最久多少（us）\n");
    if (f == FAULT_ISR_BLOCKING)
        printf("\nISR 誤用 task API 的次數（E）：%u；命令 %u 個，回應 %u 個\n", (unsigned)rows[4].misuse,
               (unsigned)rows[4].commands, (unsigned)rows[4].responses);

    printf("\nkernel 與中斷的時間（假設每次切換 %u us、每次中斷 %u us）\n", CS_US, ISR_US);
    for (size_t i = 0; i < CONFIG_COUNT; i += 4) {
        const result_t *r = &rows[i];
        printf("  %c：切換 %u 次，kernel %.1f ms，中斷 %.1f ms，idle %.1f ms，共 10000 ms\n", (char)('A' + i),
               (unsigned)r->switches, r->kernel_us / 1000.0, r->isr_us / 1000.0, r->idle_us / 1000.0);
    }

    printf("\n記憶體\n");
    print_memory();
    run(&configs[4], f);
    printf("\nE 的各 task（stack 是設定值；cpu、切換次數、等待時間是模型量到的）\n");
    print_tasks();
    printf("queue 最高佔用：batch_q %u/%u，rx_q %u/%u\n", (unsigned)batch_q.hwm, (unsigned)batch_q.len,
           (unsigned)rx_q.hwm, (unsigned)rx_q.len);
    printf("in=被切進來幾次  preempted=被搶占幾次  max_wait=ready 到真正執行最久多少 us\n");
}

/* ================= 自我檢查 ================= */
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
    result_t r[CONFIG_COUNT];
    for (size_t i = 0; i < CONFIG_COUNT; i++) r[i] = run(&configs[i], FAULT_NONE);

    printf("A 超級迴圈：25 ms 的 stats 擋住取樣，DMA 繞回的批次被作廢，遺失都有計數\n");
    CHECK(r[0].lost_reported > 50u && r[0].host_gaps == r[0].lost_reported);
    CHECK(r[0].max_cmd_lat > 20000u); /* 晚到的批次被作廢，所以 smp_lat 只算送出去的那些 */
    CHECK(r[0].crc_err == 0u && r[0].status_sent == r[0].host_statuses);

    printf("B 超級迴圈把 stats 切小段：沒有遺失，延遲降到幾 ms\n");
    CHECK(r[1].lost_reported == 0u && r[1].host_gaps == 0u);
    CHECK(r[1].max_sample_lat < 5000u);

    printf("C 兩個 task 寫同一個 TX 卻沒有 mutex：封包被蓋掉，裝置自己完全不知道\n");
    printf("  gaps=%u crc=%u STATUS %u/%u lost=%u\n", (unsigned)r[2].host_gaps, (unsigned)r[2].crc_err,
           (unsigned)r[2].status_sent, (unsigned)r[2].host_statuses, (unsigned)r[2].lost_reported);
    CHECK(r[2].lost_reported == 0u);
    CHECK(r[2].host_gaps > 0u && r[2].host_statuses < r[2].status_sent);

    printf("D 加 mutex、沒有優先權繼承：資料不再損壞，但 stats 插隊造成優先權反轉\n");
    CHECK(r[3].crc_err == 0u && r[3].status_sent == r[3].host_statuses);
    CHECK(r[3].max_block > 20000u && r[3].lost_reported > 50u);
    CHECK(r[3].host_gaps == r[3].lost_reported);

    printf("E 加優先權繼承：等待時間有上限，不再遺失\n");
    CHECK(r[4].lost_reported == 0u && r[4].host_gaps == 0u && r[4].crc_err == 0u);
    /* 繼承把等待限制在 diag 的臨界區長度之內：收集 1 ms + 15 bytes x 100 us */
    CHECK(r[4].max_block < DIAG_GATHER_US + 15u * DIAG_FMT_BYTE_US + 100u);
    CHECK(r[4].max_sample_lat < 3000u && r[4].max_cmd_lat < 3000u);
    CHECK(r[4].status_sent == r[4].host_statuses && r[4].responses == r[4].commands);
    CHECK(r[4].batch_hwm <= 2u && r[4].misuse == 0u);

    printf("F 臨界區只搬資料：等待時間再降一個數量級\n");
    CHECK(r[5].lost_reported == 0u && r[5].max_block < 200u);

    printf("RTOS 的代價：比超級迴圈多出的 stack 與切換\n");
    CHECK(stack_total(1) > stack_total(0));
    CHECK(r[4].switches > 10u * r[1].switches);

    printf("故障 no-yield（用 F，等 mutex 的時間才不會蓋過它）：被喚醒的 task 要等到下一個 tick\n");
    result_t y = run(&configs[5], FAULT_NO_YIELD);
    printf("  smp_lat %u -> %u us，cmd_lat %u -> %u us\n", (unsigned)r[5].max_sample_lat,
           (unsigned)y.max_sample_lat, (unsigned)r[5].max_cmd_lat, (unsigned)y.max_cmd_lat);
    CHECK(y.max_sample_lat >= r[5].max_sample_lat + 500u);
    CHECK(y.max_sample_lat <= r[5].max_sample_lat + TICK_US && y.lost_reported == 0u);

    printf("故障 isr-blocking：RX ISR 呼叫了會阻塞的 q_send，kernel 偵測到誤用\n");
    result_t b = run(&configs[4], FAULT_ISR_BLOCKING);
    printf("  誤用 %u 次，命令 %u 個，回應 %u 個\n", (unsigned)b.misuse, (unsigned)b.commands,
           (unsigned)b.responses);
    CHECK(b.misuse > 0u && b.responses == 0u);

    printf("ISR 裡也不能上 mutex 或 delay\n");
    run(&configs[4], FAULT_NONE);
    uint32_t before = k.misuse;
    k.in_isr = 1;
    int got = mtx_lock(&t_cmd, &tx_mtx);
    int slept = k_delay_until(&t_diag, 0u, 1000u);
    k.in_isr = 0;
    CHECK(!got && !slept && k.misuse == before + 2u);

    printf(g_fail ? "\n%d 項失敗\n" : "\n全部通過\n", g_fail);
    return g_fail ? 1 : 0;
}

static void usage(void)
{
    printf("用法：rtos_sim [--test] [--fault=NAME]\n故障名稱：");
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
            for (int kk = 0; kk < FAULT_COUNT; kk++)
                if (strcmp(argv[i] + 8, fault_names[kk]) == 0) {
                    f = (fault_t)kk;
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

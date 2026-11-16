/*
 * recovery_sim.c：用虛擬時間模擬 watchdog、重啟原因、當機循環、Flash 設定與休眠。
 *
 * 時間單位是毫秒，全部由程式推進，不讀桌機時鐘，每次執行結果都相同。
 * 模型裡的硬體都是虛構的簡化版：
 *   watchdog       獨立型用自己的時脈計數，逾時就重啟；window 型另外在「太早餵」時也重啟。
 *   重啟原因暫存器 每種重啟設一個旗標，旗標會一直累積，直到韌體寫入清除；上電時只剩上電旗標。
 *   .noinit RAM    warm reset 後保留內容，上電後是亂數。
 *   Flash          兩頁；抹除讓整頁變 0xFF，寫入只能把 1 變 0；可以指定第幾個動作進行到一半時斷電。
 *   休眠           深度休眠時 tick 停止，獨立 watchdog 繼續計數（真實晶片是否如此要查手冊）。
 * 數字（逾時 500 ms、每 100 ms 考慮餵狗、穩定 3 秒……）是教學用的假設，不代表任何晶片。
 *
 * 可以搬到板子上的是 supervisor.c、boot_guard.c、config_store.c；這個檔案只是模擬外界與韌體骨架。
 *
 * 建置：gcc -std=c11 -Wall -Wextra -Werror -O2 -o recovery_sim recovery_sim.c supervisor.c boot_guard.c config_store.c
 * 執行：./recovery_sim                印出各情境的結果
 *       ./recovery_sim --fault=NAME   換成一種錯誤設計再跑一次，NAME 見 --help
 *       ./recovery_sim --test         執行自我檢查
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "boot_guard.h"
#include "config_store.h"
#include "supervisor.h"

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

/* ================= 可切換的錯誤設計 ================= */
typedef enum {
    FAULT_NONE,
    FAULT_ISR_FEED,     /* 在 tick ISR 裡無條件餵狗 */
    FAULT_LOOP_FEED,    /* 主迴圈每一圈無條件餵狗，不看活動有沒有報到 */
    FAULT_KEEP_FLAGS,   /* 讀了重啟原因卻不清除，旗標一路累積 */
    FAULT_EARLY_STABLE, /* 一開機就宣告穩定，當機計數永遠歸零 */
    FAULT_SINGLE_SLOT,  /* 設定只存一個槽：先抹除再寫同一頁 */
    FAULT_COUNT
} fault_t;

static const char *const fault_names[FAULT_COUNT] = {
    "none", "isr-feed", "loop-feed", "keep-reset-flags", "early-stable", "single-slot",
};

static fault_t g_fault;

/* ================= 硬體模型 ================= */
enum { RCC_POR = 1u << 0, RCC_PIN = 1u << 1, RCC_IWDG = 1u << 2, RCC_WWDG = 1u << 3, RCC_SW = 1u << 4 };

typedef struct {
    bool window;         /* false 獨立 watchdog、true window watchdog */
    bool running;
    uint32_t timeout_ms; /* 自上次餵狗經過這麼久就重啟 */
    uint32_t window_ms;  /* window watchdog：經過時間小於這個值就餵，也重啟 */
    uint32_t count;      /* watchdog 自己的時脈數到的毫秒數 */
} wdg_t;

#define FLASH_PAGES 2u
#define FLASH_PAGE_SIZE 128u

typedef struct {
    uint8_t mem[FLASH_PAGES][FLASH_PAGE_SIZE];
    uint32_t erases[FLASH_PAGES];
    uint32_t ops;    /* 已開始的動作數：抹除一頁算一次，寫入一個 word 算一次 */
    uint32_t cut_at; /* 第幾個動作進行到一半時斷電；0 表示不斷電 */
    bool dead;       /* 已斷電：之後的動作都不會發生 */
} flash_sim_t;

static bool fl_erase(void *ctx, uint32_t page)
{
    flash_sim_t *f = ctx;
    if (f->dead || page >= FLASH_PAGES) return false;
    if (++f->ops == f->cut_at) {
        /* 抹除到一半斷電：只有部分 word 被抹掉，其餘還是舊內容 */
        for (uint32_t i = 0; i < FLASH_PAGE_SIZE; i += 8u) memset(&f->mem[page][i], 0xFF, 4u);
        f->dead = true;
        return false;
    }
    memset(f->mem[page], 0xFF, FLASH_PAGE_SIZE);
    f->erases[page]++;
    return true;
}

static bool fl_program(void *ctx, uint32_t page, uint32_t off, const uint8_t *src, uint32_t len)
{
    flash_sim_t *f = ctx;
    if (page >= FLASH_PAGES || off % 4u || len % 4u || off + len > FLASH_PAGE_SIZE) return false;
    for (uint32_t w = 0; w < len; w += 4u) {
        if (f->dead) return false;
        uint32_t nbytes = 4u;
        if (++f->ops == f->cut_at) { /* 寫到一半斷電：這個 word 只寫進一半 */
            nbytes = 2u;
            f->dead = true;
        }
        for (uint32_t i = 0; i < nbytes; i++)
            f->mem[page][off + w + i] &= src[w + i]; /* 寫入只能把 1 變成 0 */
        if (f->dead) return false;
    }
    return true;
}

static void fl_read(void *ctx, uint32_t page, uint32_t off, uint8_t *dst, uint32_t len)
{
    flash_sim_t *f = ctx;
    if (page >= FLASH_PAGES || off + len > FLASH_PAGE_SIZE) {
        memset(dst, 0xFF, len);
        return;
    }
    memcpy(dst, &f->mem[page][off], len);
}

static void flash_blank(flash_sim_t *f)
{
    memset(f, 0, sizeof *f);
    memset(f->mem, 0xFF, sizeof f->mem);
}

static flash_port_t flash_port(flash_sim_t *f)
{
    flash_port_t p = {fl_erase, fl_program, fl_read, f};
    return p;
}

/* 「重新上電」：斷電注入解除，Flash 內容保留 */
static void flash_power_up(flash_sim_t *f)
{
    f->dead = false;
    f->cut_at = 0;
}

/* ================= 韌體骨架 ================= */
#define STABLE_MS 3000u    /* 連續運作這麼久才算穩定，清除當機計數 */
#define TXQ_CAP 1024u      /* TX 緩衝區 */
#define SAMPLE_BYTES 12u   /* 一個取樣封包，與第五課相同 */
#define TX_BYTES_PER_MS 11u /* 115,200 baud、8N1 約 11.5 bytes/ms */
#define MAX_BOOTS 64u

enum { CMD_NONE, CMD_SET_PERIOD, CMD_SET_MODE };

typedef struct {
    uint32_t ms; /* tick ISR 遞增的毫秒計數 */
    supervisor_t sup;
    int id_sample, id_uplink, id_command;
    config_store_t store;
    config_t cfg;
    config_report_t cfg_rep;
    boot_report_t boot;
    uint32_t last_sample_ms, last_cmd_ms;
    uint32_t tx_queue, tx_sent, tx_seen;
    bool tx_irq_on, hung, stable, stall_noted, link_lost;
    uint32_t samples, dropped;
} fw_t;

typedef struct {
    uint32_t at_ms; /* 這次開機發生在第幾毫秒（真實時間） */
    uint32_t flags; /* 開機時讀到的重啟原因旗標 */
    boot_report_t boot;
    int cfg_source;
    bool upgraded;
    config_t cfg;
} boot_log_t;

typedef struct {
    bool window;
    uint32_t timeout_ms, window_ms, feed_every_ms;
    uint32_t tick_scale; /* 每 1 ms 真實時間，韌體的毫秒計數前進多少；時脈設錯時不是 1 */
} wdg_opts_t;

static const wdg_opts_t OPT_IWDG = {false, 500u, 0u, 100u, 1u};
static const wdg_opts_t OPT_WWDG = {true, 100u, 50u, 75u, 1u};

typedef struct {
    /* 硬體：重啟時依種類保留或重設 */
    wdg_t wdg;
    wdg_opts_t opt;
    uint32_t rcc;          /* 重啟原因暫存器 */
    boot_record_t noinit;  /* 不清零的 RAM 區 */
    flash_sim_t flash;
    uint32_t real_ms;
    uint32_t pending_reset;
    uint32_t lcg;
    /* 這次開機的韌體狀態：每次重啟都從零開始 */
    fw_t fw;
    /* 外界注入；0 表示不注入，各發生一次 */
    uint32_t hang_at, stall_at;
    uint32_t cmd_at;
    int cmd;
    uint16_t cmd_value;
    /* 觀察 */
    boot_log_t log[MAX_BOOTS];
    uint32_t boots;
    uint32_t total_samples, total_tx;
} device_t;

static device_t g_dev; /* 很大，放在靜態儲存區 */
static int g_trace; /* 還能印幾行追蹤；0 表示不印 */

static bool trace_ok(void)
{
    if (g_trace <= 0) return false;
    if (--g_trace == 0) {
        printf("  ……之後省略\n");
        return false;
    }
    return true;
}

static uint32_t sample_deadline(const config_t *c) { return 3u * c->period_ms + 50u; }

static void wdg_feed(device_t *d)
{
    if (!d->wdg.running) return;
    if (d->wdg.window && d->wdg.count < d->wdg.window_ms) {
        d->pending_reset = RCC_WWDG; /* 太早餵：window watchdog 也重啟 */
        return;
    }
    d->wdg.count = 0;
}

/* 韌體讀到旗標之後怎麼解讀：上電優先，其次是 watchdog，最後才是 reset 接腳。
 * 真實晶片的旗標組合與優先順序要查手冊，例如有些晶片的內部重啟也會讓接腳旗標成立。 */
static reset_cause_t decode_cause(uint32_t f)
{
    if (f & RCC_POR) return RESET_POWER_ON;
    if (f & RCC_IWDG) return RESET_WATCHDOG;
    if (f & RCC_WWDG) return RESET_WINDOW;
    if (f & RCC_SW) return RESET_SOFTWARE;
    if (f & RCC_PIN) return RESET_PIN;
    return RESET_UNKNOWN;
}

static void fw_boot(device_t *d)
{
    fw_t *fw = &d->fw;
    memset(fw, 0, sizeof *fw); /* 相當於啟動碼清零 .bss：韌體狀態從零開始 */

    /* 1. 先讀重啟原因，讀完立刻清除，下一次重啟才分得清楚 */
    uint32_t flags = d->rcc;
    if (g_fault != FAULT_KEEP_FLAGS) d->rcc = 0;

    /* 2. 開機紀錄與當機循環判斷 */
    boot_guard_start(&d->noinit, decode_cause(flags), &fw->boot);
    if (g_fault == FAULT_EARLY_STABLE) boot_guard_mark_stable(&d->noinit);

    /* 3. 設定：安全模式不相信存著的設定，但也不抹掉它，留給電腦檢查 */
    config_t stored;
    config_store_init(&fw->store, flash_port(&d->flash));
    config_load(&fw->store, &stored, &fw->cfg_rep);
    if (fw->boot.safe_mode) {
        config_defaults(&fw->cfg);
    } else {
        fw->cfg = stored;
        if (fw->cfg_rep.upgraded) config_save(&fw->store, &fw->cfg); /* 把舊格式寫回新格式 */
    }

    /* 4. 啟動 watchdog 與監督 */
    d->wdg.window = d->opt.window;
    d->wdg.timeout_ms = d->opt.timeout_ms;
    d->wdg.window_ms = d->opt.window_ms;
    d->wdg.count = 0;
    d->wdg.running = true;
    sup_init(&fw->sup, d->opt.feed_every_ms, 0u);
    fw->id_sample = sup_add(&fw->sup, "取樣", sample_deadline(&fw->cfg), 0u);
    fw->id_uplink = sup_add(&fw->sup, "上傳", 200u, 0u);
    fw->id_command = sup_add(&fw->sup, "命令", 100u, 0u);
    fw->tx_irq_on = true;

    if (d->boots < MAX_BOOTS) {
        boot_log_t *l = &d->log[d->boots];
        l->at_ms = d->real_ms;
        l->flags = flags;
        l->boot = fw->boot;
        l->cfg_source = fw->cfg_rep.source;
        l->upgraded = fw->cfg_rep.upgraded;
        l->cfg = fw->cfg;
    }
    d->boots++;
    if (trace_ok())
        printf("  [%6u ms] 開機：原因 %s，連續當機 %u，%s，設定來源 %s\n", (unsigned)d->real_ms,
               reset_cause_name(fw->boot.cause), (unsigned)fw->boot.streak,
               fw->boot.safe_mode ? "安全模式" : "一般模式",
               fw->boot.safe_mode ? "預設值（安全模式）"
               : fw->cfg_rep.source < 0 ? "預設值"
               : fw->cfg_rep.source == 0 ? "槽 A"
                                          : "槽 B");
}

static void fw_command(device_t *d)
{
    fw_t *fw = &d->fw;
    config_t c = fw->cfg;
    if (d->cmd == CMD_SET_PERIOD) c.period_ms = d->cmd_value;
    if (d->cmd == CMD_SET_MODE) c.sensor_mode = (uint8_t)d->cmd_value;
    d->cmd = CMD_NONE;
    fw->last_cmd_ms = fw->ms;
    if (!config_save(&fw->store, &c)) return; /* 無效或寫入失敗：保留原設定 */
    fw->cfg = c;
    sup_set_deadline(&fw->sup, fw->id_sample, sample_deadline(&c));
    if (fw->boot.safe_mode) boot_guard_mark_stable(&d->noinit); /* 設定修好了：下次開機離開安全模式 */
}

static void fw_main_once(device_t *d)
{
    fw_t *fw = &d->fw;
    uint32_t now = fw->ms;

    /* 1. 取樣 */
    if ((uint32_t)(now - fw->last_sample_ms) >= fw->cfg.period_ms) {
        fw->last_sample_ms = now;
        bool injected = d->hang_at && d->real_ms >= d->hang_at;
        if (injected || fw->cfg.sensor_mode == 1u) {
            /* 相當於 while (!sensor_ready()) {}：沒有上限的等待，主迴圈從此停在這裡。
             * sensor_mode 1 代表一個潛伏的 bug：這塊板子的感測器在快速模式下不回應。 */
            d->hang_at = 0;
            fw->hung = true;
            return;
        }
        if (fw->tx_queue + SAMPLE_BYTES <= TXQ_CAP)
            fw->tx_queue += SAMPLE_BYTES;
        else
            fw->dropped++;
        fw->samples++;
        d->total_samples++;
        sup_checkin(&fw->sup, fw->id_sample, now);
    }

    /* 2. 上傳：沒有資料，或資料確實有送出去，才算有進度 */
    if (d->stall_at && d->real_ms >= d->stall_at) {
        d->stall_at = 0;
        fw->tx_irq_on = false; /* 注入的 bug：TX 中斷被關掉之後再也沒有打開 */
    }
    if (fw->tx_queue == 0u || fw->tx_sent != fw->tx_seen) sup_checkin(&fw->sup, fw->id_uplink, now);
    fw->tx_seen = fw->tx_sent;

    /* 3. 命令：檢查過接收緩衝區就算報到，有沒有收到命令都一樣 */
    if (d->cmd != CMD_NONE && d->real_ms >= d->cmd_at) fw_command(d);
    fw->link_lost = fw->cfg.link_timeout_ms != 0u &&
                    (uint32_t)(now - fw->last_cmd_ms) > fw->cfg.link_timeout_ms; /* 應用狀態，不是重啟的理由 */
    sup_checkin(&fw->sup, fw->id_command, now);

    /* 4. 唯一的餵狗點 */
    if (g_fault == FAULT_LOOP_FEED) {
        wdg_feed(d);
    } else if (g_fault != FAULT_ISR_FEED) {
        if (sup_poll(&fw->sup, now)) {
            wdg_feed(d);
        } else if (fw->sup.stalled != SUP_NONE && !fw->stall_noted) {
            /* 停止餵狗之前，先把是誰停住寫進 .noinit，下次開機才查得到 */
            boot_guard_note_stall(&d->noinit, (uint32_t)fw->sup.stalled);
            fw->stall_noted = true;
            if (trace_ok())
                printf("  [%6u ms] supervisor：「%s」%u ms 沒有進度，停止餵狗\n", (unsigned)d->real_ms,
                       fw->sup.task[fw->sup.stalled].name,
                       (unsigned)fw->sup.task[fw->sup.stalled].max_silence_ms);
        }
    }

    /* 5. 穩定運作一段時間才清除當機計數；安全模式要等設定修好 */
    if (!fw->stable && !fw->boot.safe_mode && now >= STABLE_MS) {
        boot_guard_mark_stable(&d->noinit);
        fw->stable = true;
    }
}

static void device_reset(device_t *d, uint32_t flag)
{
    if (flag == RCC_POR) {
        d->rcc = RCC_POR; /* 上電：其他旗標一併清除 */
        uint32_t junk[sizeof d->noinit / 4u];
        for (uint32_t i = 0; i < sizeof d->noinit / 4u; i++) {
            d->lcg = d->lcg * 1664525u + 1013904223u;
            junk[i] = d->lcg;
        }
        memcpy(&d->noinit, junk, sizeof d->noinit); /* 上電後 SRAM 是亂數 */
    } else {
        d->rcc |= flag; /* warm reset：旗標累積，.noinit 保留 */
    }
    if (flag != RCC_POR && trace_ok())
        printf("  [%6u ms] 重啟：%s\n", (unsigned)d->real_ms,
               flag == RCC_IWDG   ? "watchdog 逾時"
               : flag == RCC_WWDG ? "window watchdog"
               : flag == RCC_PIN  ? "按下 reset 鍵"
                                  : "軟體要求");
    d->wdg.running = false; /* 本模型：重啟後 watchdog 停止，等韌體再啟動 */
    d->pending_reset = 0;
    fw_boot(d);
}

/* 建立一台還沒上電的裝置；Flash 是空白的，測試可以先寫入內容再上電 */
static void device_create(device_t *d, const wdg_opts_t *opt)
{
    memset(d, 0, sizeof *d);
    d->opt = *opt;
    d->lcg = 12345u;
    flash_blank(&d->flash);
}

static void device_step(device_t *d)
{
    fw_t *fw = &d->fw;
    d->real_ms++;

    /* tick ISR */
    fw->ms += d->opt.tick_scale;
    if (g_fault == FAULT_ISR_FEED) wdg_feed(d);

    /* 主迴圈的一圈；卡住時不會執行 */
    if (!fw->hung && !d->pending_reset) fw_main_once(d);

    /* TX ISR：中斷開著才會把資料送出去 */
    if (fw->tx_irq_on) {
        uint32_t n = fw->tx_queue < TX_BYTES_PER_MS ? fw->tx_queue : TX_BYTES_PER_MS;
        fw->tx_queue -= n;
        fw->tx_sent += n;
        d->total_tx += n;
    }

    /* watchdog 用自己的時脈計數，不受主迴圈影響 */
    if (d->wdg.running && !d->pending_reset && ++d->wdg.count >= d->wdg.timeout_ms)
        d->pending_reset = d->wdg.window ? RCC_WWDG : RCC_IWDG;
    if (d->pending_reset) device_reset(d, d->pending_reset);
}

static void device_run(device_t *d, uint32_t ms)
{
    for (uint32_t i = 0; i < ms; i++) device_step(d);
}

/* ================= 情境 1：卡死偵測 ================= */
enum { INJECT_HANG, INJECT_STALL };

typedef struct {
    bool detected;
    uint32_t inject_ms, reset_ms;
    reset_cause_t cause;
    uint32_t stalled_task; /* 重啟後回報的停滯活動 */
    uint32_t tail_bytes;   /* 最後 1 秒送出的 bytes：資料流有沒有恢復 */
} hang_result_t;

static hang_result_t hang_case(int kind)
{
    device_t *d = &g_dev;
    hang_result_t r;
    memset(&r, 0, sizeof r);
    device_create(d, &OPT_IWDG);
    device_reset(d, RCC_POR);
    device_run(d, 2000u);
    r.inject_ms = d->real_ms + 1u;
    if (kind == INJECT_HANG)
        d->hang_at = r.inject_ms;
    else
        d->stall_at = r.inject_ms;
    uint32_t boots = d->boots;
    device_run(d, 3000u);
    uint32_t tx_before_tail = d->total_tx;
    device_run(d, 1000u);
    r.tail_bytes = d->total_tx - tx_before_tail;
    r.detected = d->boots > boots;
    if (r.detected && boots < MAX_BOOTS) {
        r.reset_ms = d->log[boots].at_ms;
        r.cause = d->log[boots].boot.cause;
        r.stalled_task = d->log[boots].boot.stalled_task;
    }
    return r;
}

static const char *task_name(uint32_t id)
{
    static const char *const names[] = {"取樣", "上傳", "命令"};
    if (id == BOOT_NO_TASK) return "沒有紀錄（主迴圈停住，supervisor 也停了）";
    return id < 3u ? names[id] : "?";
}

static void print_hang(const char *title, const hang_result_t *r)
{
    printf("%s：注入於 %u ms，", title, (unsigned)r->inject_ms);
    if (r->detected)
        printf("重啟於 %u ms（%u ms 後），原因 %s，回報停住的活動：%s\n", (unsigned)r->reset_ms,
               (unsigned)(r->reset_ms - r->inject_ms), reset_cause_name(r->cause),
               task_name(r->stalled_task));
    else
        printf("沒有重啟：watchdog 沒有發現\n");
    printf("    最後 1 秒送出 %u bytes（正常約 1,200）\n", (unsigned)r->tail_bytes);
}

/* ================= 情境 2：重啟原因 ================= */
static void cause_sequence(device_t *d)
{
    device_create(d, &OPT_IWDG);
    device_reset(d, RCC_POR); /* 第 1 次開機：上電 */
    device_run(d, 2000u);
    d->hang_at = d->real_ms + 1u; /* 第 2 次開機：watchdog */
    device_run(d, 4000u);         /* 之後穩定運作超過 3 秒 */
    device_reset(d, RCC_PIN);     /* 第 3 次開機：reset 鍵 */
    device_run(d, 1000u);
    device_reset(d, RCC_SW); /* 第 4 次開機：軟體要求，例如更新後 */
    device_run(d, 1000u);
}

static void flags_text(uint32_t f, char *out, size_t n)
{
    static const char *const names[] = {"POR", "PIN", "IWDG", "WWDG", "SW"};
    out[0] = 0;
    for (unsigned i = 0; i < 5u; i++)
        if (f & (1u << i)) {
            size_t len = strlen(out);
            snprintf(out + len, n - len, "%s%s", len ? "|" : "", names[i]);
        }
    if (!out[0]) snprintf(out, n, "-");
}

/* ================= 情境 3：當機循環 ================= */
static void crash_loop(device_t *d)
{
    device_create(d, &OPT_IWDG);
    device_reset(d, RCC_POR);
    device_run(d, 1000u);
    d->cmd = CMD_SET_MODE; /* 電腦送來一份通過檢查、卻會觸發 bug 的設定 */
    d->cmd_value = 1u;
    d->cmd_at = d->real_ms + 1u;
    device_run(d, 9000u);
}

static uint32_t first_safe_boot(const device_t *d)
{
    for (uint32_t i = 0; i < d->boots && i < MAX_BOOTS; i++)
        if (d->log[i].boot.safe_mode) return i + 1u;
    return 0;
}

/* ================= 情境 4：設定寫入時斷電 ================= */
static const config_t CFG_X = {20u, 0u, 3000u};
static const config_t CFG_Y = {50u, 0u, 5000u};
static const config_t CFG_Z = {100u, 0u, 0u};

/* 錯誤示範：只有一個槽，先抹除再寫回同一頁 */
static bool bad_save_single_slot(config_store_t *st, const config_t *c)
{
    if (!config_valid(c)) return false;
    uint8_t rec[CONFIG_RECORD_MAX];
    uint32_t n = config_encode(c, st->seq + 1u, rec);
    if (!st->flash.erase(st->flash.ctx, 0u)) return false;
    if (!st->flash.program(st->flash.ctx, 0u, 0u, rec, n)) return false;
    st->active = 0;
    st->seq++;
    return true;
}

typedef bool (*save_fn_t)(config_store_t *st, const config_t *c);

enum { R_OLD, R_NEW, R_DEFAULTS, R_OTHER };
static const char *const result_names[] = {"舊", "新", "預設", "錯"};

typedef struct {
    int result[16]; /* 每個斷電點的結果；最後一格是沒有斷電 */
    uint32_t points;
    uint32_t ops_per_save;
    bool completed; /* 沒有斷電的那次寫入回報成功 */
} sweep_t;

static int classify(const config_t *c, const config_t *old, const config_t *neu)
{
    config_t def;
    config_defaults(&def);
    if (config_equal(c, neu)) return R_NEW;
    if (old && config_equal(c, old)) return R_OLD;
    if (config_equal(c, &def)) return R_DEFAULTS;
    return R_OTHER;
}

/* 先依序寫入 before[0..nbefore)，再寫 target，並在 target 的每一個動作斷電一次 */
static sweep_t power_cut_sweep(save_fn_t save, const config_t *before, uint32_t nbefore,
                               const config_t *target)
{
    static flash_sim_t f;
    sweep_t s;
    memset(&s, 0, sizeof s);
    for (uint32_t cut = 1; cut < 16u; cut++) {
        config_store_t st;
        config_t c;
        config_report_t rep;
        flash_blank(&f);
        config_store_init(&st, flash_port(&f));
        config_load(&st, &c, &rep);
        for (uint32_t i = 0; i < nbefore; i++) save(&st, &before[i]);

        uint32_t start = f.ops;
        f.cut_at = start + cut;
        bool ok = save(&st, target);
        bool was_cut = f.dead;
        if (!was_cut) s.ops_per_save = f.ops - start;
        flash_power_up(&f);

        config_store_init(&st, flash_port(&f)); /* 重新開機 */
        config_load(&st, &c, &rep);
        s.result[s.points++] = classify(&c, nbefore ? &before[nbefore - 1u] : NULL, target);
        if (!was_cut) {
            s.completed = ok;
            break;
        }
    }
    return s;
}

static bool sweep_only(const sweep_t *s, int a, int b)
{
    for (uint32_t i = 0; i < s->points; i++)
        if (s->result[i] != a && s->result[i] != b) return false;
    return true;
}

static uint32_t sweep_count(const sweep_t *s, int r)
{
    uint32_t n = 0;
    for (uint32_t i = 0; i < s->points; i++) n += s->result[i] == r;
    return n;
}

static void print_sweep(const char *title, const sweep_t *s)
{
    printf("%s\n   ", title);
    for (uint32_t i = 0; i < s->points; i++) {
        if (i + 1u == s->points)
            printf(" 不斷電:%s", result_names[s->result[i]]);
        else
            printf(" %u:%s", (unsigned)(i + 1u), result_names[s->result[i]]);
    }
    printf("\n");
}

/* 舊版韌體寫的 v1 記錄，或任意內容的記錄：用來測試升級與錯誤處理 */
static void put16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
}
static void put32(uint8_t *p, uint32_t v)
{
    put16(p, (uint16_t)v);
    put16(p + 2, (uint16_t)(v >> 16));
}

static void write_raw_record(flash_sim_t *f, uint32_t page, uint16_t version, uint16_t len,
                             const uint8_t *payload, uint32_t seq)
{
    uint8_t rec[CONFIG_RECORD_MAX];
    uint32_t padded = (len + 3u) & ~3u;
    memset(rec, 0, sizeof rec);
    put32(&rec[0], CONFIG_MAGIC);
    put16(&rec[4], version);
    put16(&rec[6], len);
    put32(&rec[8], seq);
    memcpy(&rec[12], payload, len);
    put32(&rec[12u + padded], crc32_ieee(rec, 12u + padded));
    fl_erase(f, page);
    fl_program(f, page, 0u, rec, 12u + padded + 4u);
}

static void write_v1(flash_sim_t *f, uint32_t page, uint16_t period, uint8_t mode, uint32_t seq)
{
    uint8_t p[4] = {(uint8_t)period, (uint8_t)(period >> 8), mode, 0u};
    write_raw_record(f, page, 1u, 4u, p, seq);
}

static void write_v2(flash_sim_t *f, uint32_t page, const config_t *c, uint32_t seq)
{
    uint8_t p[6] = {(uint8_t)c->period_ms, (uint8_t)(c->period_ms >> 8), c->sensor_mode, 0u,
                    (uint8_t)c->link_timeout_ms, (uint8_t)(c->link_timeout_ms >> 8)};
    write_raw_record(f, page, 2u, 6u, p, seq);
}

/* ================= 情境 5：window watchdog 與時脈 ================= */
typedef struct {
    uint32_t resets;
    reset_cause_t first_cause;
} window_result_t;

static window_result_t window_case(const wdg_opts_t *opt, uint32_t tick_scale)
{
    device_t *d = &g_dev;
    wdg_opts_t o = *opt;
    o.tick_scale = tick_scale;
    device_create(d, &o);
    device_reset(d, RCC_POR);
    device_run(d, 3000u);
    window_result_t r = {d->boots - 1u, RESET_UNKNOWN};
    if (d->boots > 1u) r.first_cause = d->log[1].boot.cause;
    return r;
}

/* ================= 情境 6：休眠與 watchdog、tick ================= */
typedef struct {
    uint32_t resets, wakes;
    uint32_t lag_ms; /* 最後一次醒來時，tick 比這次開機以來的真實時間落後多少 */
} sleep_result_t;

/* 每次醒來工作 2 ms（報到、餵狗），然後深度休眠 rtc_ms 毫秒，由 RTC 鬧鐘叫醒。
 * rtc_ms = 0 表示沒有設定週期喚醒，只等外部事件，而這段期間沒有外部事件。
 * 深度休眠時 tick 停止，獨立 watchdog 繼續數；compensate 表示醒來時用 RTC 補上睡掉的時間。 */
static sleep_result_t sleep_case(uint32_t total_ms, uint32_t rtc_ms, bool compensate)
{
    sleep_result_t r = {0, 0, 0};
    const uint32_t timeout = 500u, awake_ms = 2u;
    uint32_t wdg = 0, awake_left = awake_ms, slept = 0, tick = 0, boot_t = 0;
    bool asleep = false;
    for (uint32_t t = 0; t < total_ms; t++) {
        wdg++;
        if (!asleep) {
            tick++; /* 醒著時 tick 中斷照常 */
            if (--awake_left == 0u) {
                wdg = 0; /* 所有活動都報到之後才餵 */
                asleep = true;
                slept = 0;
            }
        } else {
            slept++;
            if (rtc_ms && slept >= rtc_ms) {
                r.wakes++;
                if (compensate) tick += slept;
                r.lag_ms = (t + 1u - boot_t) - tick;
                asleep = false;
                awake_left = awake_ms;
            }
        }
        if (wdg >= timeout) { /* 睡太久：watchdog 在休眠中重啟，重新開機後又回去睡 */
            r.resets++;
            tick = 0;
            boot_t = t + 1u;
            wdg = 0;
            asleep = false;
            awake_left = awake_ms;
        }
    }
    return r;
}

/* ================= 印出各情境 ================= */
static void report(void)
{
    device_t *d = &g_dev;
    printf("== 1. 卡死偵測：獨立 watchdog 逾時 500 ms，supervisor 每 100 ms 考慮餵一次 ==\n");
    hang_result_t h = hang_case(INJECT_HANG);
    print_hang("主迴圈卡死（等待感測器沒有上限）", &h);
    h = hang_case(INJECT_STALL);
    print_hang("上傳停滯（TX 中斷被關掉，主迴圈照跑）", &h);

    printf("\n== 2. 重啟原因：上電 → 卡死 → 按 reset 鍵 → 軟體重啟 ==\n");
    cause_sequence(d);
    for (uint32_t i = 0; i < d->boots && i < MAX_BOOTS; i++) {
        char ft[40];
        flags_text(d->log[i].flags, ft, sizeof ft);
        printf("  第 %u 次開機  %5u ms  旗標 %-14s 連續當機 %u  判讀為 %s\n", (unsigned)(i + 1u),
               (unsigned)d->log[i].at_ms, ft, (unsigned)d->log[i].boot.streak,
               reset_cause_name(d->log[i].boot.cause));
    }

    printf("\n== 3. 當機循環：第 1 秒收到 sensor_mode=1，這個設定會讓取樣卡死 ==\n");
    g_trace = 15;
    crash_loop(d);
    g_trace = 0;
    uint32_t safe = first_safe_boot(d);
    config_t c;
    config_report_t rep;
    config_store_t st;
    config_store_init(&st, flash_port(&d->flash));
    config_load(&st, &c, &rep);
    if (safe)
        printf("  第 %u 次開機進入安全模式，10 秒內共開機 %u 次；Flash 裡的設定仍是 sensor_mode=%u，等電腦修正\n",
               (unsigned)safe, (unsigned)d->boots, (unsigned)c.sensor_mode);
    else
        printf("  10 秒內開機 %u 次，沒有進入安全模式；%s\n", (unsigned)d->boots,
               d->fw.hung ? "裝置停在卡死狀態" : "仍在反覆重啟");

    printf("\n== 4. 設定寫入時斷電：每一個 Flash 動作都試一次 ==\n");
    save_fn_t save = g_fault == FAULT_SINGLE_SLOT ? bad_save_single_slot : config_save;
    sweep_t s = power_cut_sweep(save, &CFG_X, 1u, &CFG_Y);
    printf("  一次寫入共 %u 個動作：抹除 1 次、寫入 %u 個 word，CRC 最後寫\n", (unsigned)s.ops_per_save,
           (unsigned)(s.ops_per_save - 1u));
    print_sweep(g_fault == FAULT_SINGLE_SLOT ? "  只有一個槽：已有設定 X，改寫成 Y"
                                             : "  兩個槽：已有設定 X，寫入 Y",
                &s);

    printf("\n== 5. 格式升級：槽 A 有舊版韌體寫的 v1 設定 ==\n");
    device_create(d, &OPT_IWDG);
    write_v1(&d->flash, 0u, 20u, 0u, 5u);
    g_trace = 15;
    device_reset(d, RCC_POR);
    g_trace = 0;
    printf("  讀到 v%u、升級=%s；period=%u、link_timeout=%u（v1 沒有這個欄位，填預設值）\n",
           (unsigned)d->fw.cfg_rep.version[0], d->log[0].upgraded ? "是" : "否",
           (unsigned)d->fw.cfg.period_ms, (unsigned)d->fw.cfg.link_timeout_ms);
    config_store_init(&st, flash_port(&d->flash));
    config_load(&st, &c, &rep);
    printf("  寫回後：槽 A %s v%u、槽 B %s v%u，生效的是槽 %c，序號 %u\n", slot_state_name(rep.slot[0]),
           (unsigned)rep.version[0], slot_state_name(rep.slot[1]), (unsigned)rep.version[1],
           rep.source == 1 ? 'B' : 'A', (unsigned)rep.seq);

    printf("\n== 6. window watchdog（逾時 100 ms、50 ms 前餵也重啟）與時脈 ==\n");
    window_result_t w = window_case(&OPT_WWDG, 1u);
    printf("  window，時脈正確：      3 秒內重啟 %u 次\n", (unsigned)w.resets);
    w = window_case(&OPT_WWDG, 2u);
    printf("  window，tick 快了一倍： 3 秒內重啟 %u 次，第一次原因 %s\n", (unsigned)w.resets,
           reset_cause_name(w.first_cause));
    w = window_case(&OPT_IWDG, 2u);
    printf("  獨立，tick 快了一倍：   3 秒內重啟 %u 次\n", (unsigned)w.resets);

    printf("\n== 7. 深度休眠 5 秒：tick 停止、獨立 watchdog 繼續數 ==\n");
    sleep_result_t z = sleep_case(5000u, 0u, false);
    printf("  沒有週期喚醒：           重啟 %u 次\n", (unsigned)z.resets);
    z = sleep_case(5000u, 600u, false);
    printf("  RTC 每 600 ms 喚醒：     重啟 %u 次\n", (unsigned)z.resets);
    z = sleep_case(5000u, 400u, false);
    printf("  RTC 每 400 ms 喚醒：     重啟 %u 次，最後一次醒來時 tick 落後 %u ms\n",
           (unsigned)z.resets, (unsigned)z.lag_ms);
    z = sleep_case(5000u, 400u, true);
    printf("  同上，醒來用 RTC 補時間：tick 落後 %u ms\n", (unsigned)z.lag_ms);
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

static void test_supervisor(void)
{
    printf("supervisor：所有活動都報到才餵，而且每 100 ms 最多一次\n");
    supervisor_t s;
    sup_init(&s, 100u, 0u);
    int a = sup_add(&s, "a", 50u, 0u);
    int b = sup_add(&s, "b", 300u, 0u);
    uint32_t feeds = 0;
    for (uint32_t t = 1; t <= 1000u; t++) {
        sup_checkin(&s, a, t);
        if (t % 200u == 0u) sup_checkin(&s, b, t);
        feeds += sup_poll(&s, t);
    }
    CHECK(feeds == 10u && s.stalled == SUP_NONE);
    printf("supervisor：一個活動停住就不再餵，即使之後又恢復\n");
    for (uint32_t t = 1001; t <= 2000u; t++) {
        if (t < 1100u || t > 1500u) sup_checkin(&s, a, t);
        sup_checkin(&s, b, t);
        feeds += sup_poll(&s, t);
    }
    CHECK(s.stalled == a && feeds <= 11u);
    printf("supervisor：計數器回繞時照樣正確\n");
    sup_init(&s, 100u, 0xFFFFFF00u);
    a = sup_add(&s, "a", 50u, 0xFFFFFF00u);
    feeds = 0;
    for (uint32_t i = 1; i <= 1000u; i++) {
        uint32_t t = 0xFFFFFF00u + i;
        sup_checkin(&s, a, t);
        feeds += sup_poll(&s, t);
    }
    CHECK(feeds == 10u && s.stalled == SUP_NONE);
}

static void test_hangs(void)
{
    printf("正常運作 10 秒、沒有任何命令：不重啟，只標示斷線\n");
    device_t *d = &g_dev;
    device_create(d, &OPT_IWDG);
    device_reset(d, RCC_POR);
    device_run(d, 10000u);
    CHECK(d->boots == 1u && d->fw.link_lost && d->total_samples == 1000u);

    printf("報到式監督：主迴圈卡死被偵測，重啟後資料流恢復\n");
    hang_result_t h = hang_case(INJECT_HANG);
    CHECK(h.detected && h.cause == RESET_WATCHDOG);
    CHECK(h.reset_ms - h.inject_ms <= 600u);
    CHECK(h.stalled_task == BOOT_NO_TASK && h.tail_bytes >= 1000u);
    printf("報到式監督：只有上傳停住也被偵測，而且知道是誰\n");
    h = hang_case(INJECT_STALL);
    CHECK(h.detected && h.cause == RESET_WATCHDOG && h.stalled_task == 1u);
    CHECK(h.reset_ms - h.inject_ms <= 800u && h.tail_bytes >= 1000u);

    printf("錯誤設計：主迴圈每圈都餵，卡死抓得到，上傳停滯抓不到\n");
    g_fault = FAULT_LOOP_FEED;
    h = hang_case(INJECT_HANG);
    CHECK(h.detected);
    h = hang_case(INJECT_STALL);
    CHECK(!h.detected && h.tail_bytes == 0u);
    printf("錯誤設計：在 tick ISR 裡餵，主迴圈卡死也抓不到\n");
    g_fault = FAULT_ISR_FEED;
    h = hang_case(INJECT_HANG);
    CHECK(!h.detected && h.tail_bytes == 0u);
    h = hang_case(INJECT_STALL);
    CHECK(!h.detected);
    g_fault = FAULT_NONE;
}

static void test_reset_causes(void)
{
    printf("重啟原因：上電、watchdog、reset 鍵、軟體，各自判讀正確\n");
    device_t *d = &g_dev;
    cause_sequence(d);
    CHECK(d->boots == 4u);
    CHECK(d->log[0].boot.cause == RESET_POWER_ON && d->log[1].boot.cause == RESET_WATCHDOG);
    CHECK(d->log[2].boot.cause == RESET_PIN && d->log[3].boot.cause == RESET_SOFTWARE);
    CHECK(d->log[1].boot.streak == 1u && d->log[2].boot.streak == 0u); /* 穩定後歸零，reset 鍵不算當機 */
    printf("錯誤設計：旗標沒有清除，watchdog 重啟被當成上電，reset 鍵也判讀錯\n");
    g_fault = FAULT_KEEP_FLAGS;
    cause_sequence(d);
    g_fault = FAULT_NONE;
    CHECK(d->log[1].boot.cause == RESET_POWER_ON);
    CHECK(d->log[2].boot.cause != RESET_PIN);
}

static void test_crash_loop(void)
{
    printf("當機循環：連續三次 watchdog 重啟後，第 4 次開機進入安全模式並保持穩定\n");
    device_t *d = &g_dev;
    crash_loop(d);
    CHECK(first_safe_boot(d) == 4u);
    CHECK(d->boots == 4u && d->fw.boot.safe_mode && d->fw.cfg.sensor_mode == 0u);
    CHECK(d->fw.cfg_rep.source >= 0); /* 會出事的那份設定還在，沒有被抹掉 */
    printf("安全模式：電腦改回 sensor_mode=0 後，按 reset 鍵回到一般模式\n");
    d->cmd = CMD_SET_MODE;
    d->cmd_value = 0u;
    d->cmd_at = d->real_ms + 1u;
    device_run(d, 100u);
    device_reset(d, RCC_PIN);
    device_run(d, 5000u);
    CHECK(d->boots == 5u && !d->fw.boot.safe_mode && d->fw.cfg.sensor_mode == 0u);

    printf("偶發卡死：每次間隔超過穩定時間，計數不累積，不進安全模式\n");
    device_create(d, &OPT_IWDG);
    device_reset(d, RCC_POR);
    for (int i = 0; i < 4; i++) {
        device_run(d, 4000u);
        d->hang_at = d->real_ms + 1u;
    }
    device_run(d, 2000u);
    CHECK(d->boots == 5u && first_safe_boot(d) == 0u && d->fw.boot.streak == 1u);

    printf("錯誤設計：一開機就宣告穩定，當機循環永遠不會進入安全模式\n");
    g_fault = FAULT_EARLY_STABLE;
    crash_loop(d);
    g_fault = FAULT_NONE;
    CHECK(first_safe_boot(d) == 0u && d->boots > 10u);
    printf("錯誤設計：在 ISR 裡餵狗時，會卡死的設定讓裝置停住且永遠不重啟\n");
    g_fault = FAULT_ISR_FEED;
    crash_loop(d);
    g_fault = FAULT_NONE;
    CHECK(d->boots == 1u && d->fw.hung);
}

static void test_config(void)
{
    static flash_sim_t f;
    config_store_t st;
    config_t c, def;
    config_report_t rep;
    config_defaults(&def);

    printf("CRC-32 檢查值\n");
    CHECK(crc32_ieee((const uint8_t *)"123456789", 9u) == 0xCBF43926u);

    printf("全新的 Flash：兩個槽都空白，使用預設值\n");
    flash_blank(&f);
    config_store_init(&st, flash_port(&f));
    config_load(&st, &c, &rep);
    CHECK(rep.source == -1 && rep.slot[0] == SLOT_EMPTY && rep.slot[1] == SLOT_EMPTY);
    CHECK(config_equal(&c, &def));

    printf("輪流寫入兩個槽，序號遞增\n");
    CHECK(config_save(&st, &CFG_X) && st.active == 0 && st.seq == 1u);
    CHECK(config_save(&st, &CFG_Y) && st.active == 1 && st.seq == 2u);
    CHECK(config_save(&st, &CFG_Z) && st.active == 0 && st.seq == 3u);
    config_store_init(&st, flash_port(&f));
    config_load(&st, &c, &rep);
    CHECK(rep.source == 0 && rep.seq == 3u && config_equal(&c, &CFG_Z));

    printf("無效設定不寫入，Flash 完全沒有動作\n");
    config_t bad = CFG_X;
    bad.period_ms = 0u;
    uint32_t ops = f.ops;
    CHECK(!config_save(&st, &bad) && f.ops == ops);

    printf("寫入時在每一個動作斷電：開機後永遠是舊設定或新設定\n");
    sweep_t s = power_cut_sweep(config_save, &CFG_X, 1u, &CFG_Y);
    CHECK(s.ops_per_save == 7u && s.points == 8u && s.completed);
    CHECK(sweep_only(&s, R_OLD, R_NEW) && s.result[s.points - 1u] == R_NEW);
    printf("第三次寫入會覆蓋最舊的那一槽：斷電時仍是上一份或新的這份，不會退回更早的\n");
    config_t xy[2] = {CFG_X, CFG_Y};
    s = power_cut_sweep(config_save, xy, 2u, &CFG_Z);
    CHECK(s.completed && sweep_only(&s, R_OLD, R_NEW) && s.result[s.points - 1u] == R_NEW);
    printf("第一次寫入就斷電：沒有舊設定可退，使用預設值\n");
    s = power_cut_sweep(config_save, NULL, 0u, &CFG_Y);
    CHECK(s.completed && s.result[s.points - 1u] == R_NEW && sweep_count(&s, R_DEFAULTS) == s.points - 1u);
    printf("錯誤示範：只有一個槽，斷電就失去上一份好設定\n");
    s = power_cut_sweep(bad_save_single_slot, &CFG_X, 1u, &CFG_Y);
    CHECK(sweep_count(&s, R_DEFAULTS) >= 5u);

    printf("較新的槽數值超出範圍：退回另一槽；兩槽都無效：預設值\n");
    flash_blank(&f);
    write_v2(&f, 0u, &CFG_X, 1u);
    write_v2(&f, 1u, &bad, 2u);
    config_store_init(&st, flash_port(&f));
    config_load(&st, &c, &rep);
    CHECK(rep.source == 0 && rep.slot[1] == SLOT_BAD_VALUE && config_equal(&c, &CFG_X));
    write_v2(&f, 0u, &bad, 3u);
    config_load(&st, &c, &rep);
    CHECK(rep.source == -1 && config_equal(&c, &def));

    printf("位元翻轉後數值仍在範圍內：只有 CRC 抓得到\n");
    flash_blank(&f);
    config_store_init(&st, flash_port(&f));
    config_save(&st, &CFG_X);
    config_save(&st, &CFG_Y);
    f.mem[1][12] ^= 0x04u; /* 槽 B 的 period：50 變成 54 */
    config_store_init(&st, flash_port(&f));
    config_load(&st, &c, &rep);
    CHECK(rep.slot[1] == SLOT_BAD_CRC && rep.source == 0 && config_equal(&c, &CFG_X));

    printf("序號回繞：0x00000000 比 0xFFFFFFFF 新\n");
    flash_blank(&f);
    write_v2(&f, 0u, &CFG_X, 0xFFFFFFFFu);
    write_v2(&f, 1u, &CFG_Y, 0u);
    config_store_init(&st, flash_port(&f));
    config_load(&st, &c, &rep);
    CHECK(rep.source == 1 && config_equal(&c, &CFG_Y));
    CHECK(config_save(&st, &CFG_Z) && st.active == 0 && st.seq == 1u);

    printf("看不懂的新版本：不猜，退回另一槽\n");
    flash_blank(&f);
    write_v2(&f, 0u, &CFG_X, 1u);
    uint8_t future[6] = {50u, 0u, 0u, 0u, 0u, 0u};
    write_raw_record(&f, 1u, 9u, 6u, future, 2u);
    config_store_init(&st, flash_port(&f));
    config_load(&st, &c, &rep);
    CHECK(rep.slot[1] == SLOT_BAD_FORMAT && rep.source == 0 && config_equal(&c, &CFG_X));

    printf("v1 升級：新欄位填預設值，開機時寫回 v2；寫回時斷電也不會遺失\n");
    device_t *d = &g_dev;
    device_create(d, &OPT_IWDG);
    write_v1(&d->flash, 0u, 20u, 0u, 5u);
    device_reset(d, RCC_POR);
    CHECK(d->log[0].upgraded && d->fw.cfg.period_ms == 20u && d->fw.cfg.link_timeout_ms == 3000u);
    config_store_init(&st, flash_port(&d->flash));
    config_load(&st, &c, &rep);
    CHECK(rep.source == 1 && rep.version[1] == 2u && rep.seq == 6u && !rep.upgraded);
    CHECK(rep.slot[0] == SLOT_OK && rep.version[0] == 1u); /* 舊的那份留著當備份 */
    int upgrade_ok = 1;
    for (uint32_t cut = 1; cut <= 7u; cut++) {
        flash_blank(&f);
        write_v1(&f, 0u, 20u, 0u, 5u);
        config_store_init(&st, flash_port(&f));
        config_load(&st, &c, &rep);
        f.cut_at = f.ops + cut;
        config_save(&st, &c);
        flash_power_up(&f);
        config_store_init(&st, flash_port(&f));
        config_load(&st, &c, &rep);
        upgrade_ok &= c.period_ms == 20u && c.link_timeout_ms == 3000u && rep.source >= 0;
    }
    CHECK(upgrade_ok);

    printf("抹寫平均：100 次寫入，兩頁各抹除 50 次，每次都讀回最新的一份\n");
    flash_blank(&f);
    config_store_init(&st, flash_port(&f));
    config_load(&st, &c, &rep);
    int all_ok = 1;
    for (uint16_t i = 0; i < 100u; i++) {
        config_t v = CFG_X;
        v.period_ms = (uint16_t)(1u + i);
        all_ok &= config_save(&st, &v);
        config_store_t st2;
        config_store_init(&st2, flash_port(&f));
        config_load(&st2, &c, &rep);
        all_ok &= config_equal(&c, &v);
    }
    CHECK(all_ok && f.erases[0] == 50u && f.erases[1] == 50u);
}

static void test_window_and_sleep(void)
{
    printf("window watchdog：時脈正確不重啟；tick 快一倍時太早餵而重啟；獨立 watchdog 看不出來\n");
    window_result_t w = window_case(&OPT_WWDG, 1u);
    CHECK(w.resets == 0u);
    w = window_case(&OPT_WWDG, 2u);
    CHECK(w.resets > 0u && w.first_cause == RESET_WINDOW);
    w = window_case(&OPT_IWDG, 2u);
    CHECK(w.resets == 0u);

    printf("深度休眠：睡超過逾時就被重啟；RTC 喚醒要比逾時短；tick 要用 RTC 補\n");
    sleep_result_t z = sleep_case(5000u, 0u, false);
    CHECK(z.resets >= 9u);
    z = sleep_case(5000u, 600u, false);
    CHECK(z.resets >= 9u);
    z = sleep_case(5000u, 400u, false);
    CHECK(z.resets == 0u && z.lag_ms > 4000u);
    z = sleep_case(5000u, 400u, true);
    CHECK(z.resets == 0u && z.wakes >= 12u && z.lag_ms == 0u);
}

static int run_tests(void)
{
    g_fault = FAULT_NONE;
    test_supervisor();
    test_hangs();
    test_reset_causes();
    test_crash_loop();
    test_config();
    test_window_and_sleep();
    printf(g_fail ? "\n%d 項失敗\n" : "\n全部通過\n", g_fail);
    return g_fail ? 1 : 0;
}

static void usage(void)
{
    printf("用法：recovery_sim [--test] [--fault=NAME]\n錯誤設計：");
    for (int i = 1; i < FAULT_COUNT; i++) printf(" %s", fault_names[i]);
    printf("\n");
}

int main(int argc, char **argv)
{
    console_utf8();
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--test") == 0) return run_tests();
        if (strncmp(argv[i], "--fault=", 8) == 0) {
            int found = 0;
            for (int k = 0; k < FAULT_COUNT; k++)
                if (strcmp(argv[i] + 8, fault_names[k]) == 0) {
                    g_fault = (fault_t)k;
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
    if (g_fault != FAULT_NONE) printf("錯誤設計：%s\n\n", fault_names[g_fault]);
    report();
    return 0;
}

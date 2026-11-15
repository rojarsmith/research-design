/*
 * boot_sim.c：在桌機上模擬「reset 到 main」的資料搬運。
 *
 * 這不是 CPU 模擬器，不執行任何機器碼。它只模擬四件事：
 *   1. 硬體在 reset 時從向量表讀出初始 SP 與 reset handler 地址；
 *   2. linker 決定每個區段的 LMA（存放位置）與 VMA（執行位置）；
 *   3. 啟動碼把 .data 初值從 Flash 搬到 SRAM、把 .bss 清零；
 *   4. 不清零的 .noinit 區如何在 warm reset 後保留資料。
 *
 * 地址配置參考 Armv7-M 預設記憶體映射：Code 區從 0x00000000 開始，
 * SRAM 區從 0x20000000 開始。實際晶片的 Flash 起點、容量與 boot 別名
 * 由廠商決定，必須查該晶片的 reference manual。
 *
 * 建置：gcc -std=c11 -Wall -Wextra -Werror -O2 -o boot_sim boot_sim.c
 * 執行：./boot_sim                正常上電，印出 map 與啟動追蹤
 *       ./boot_sim --fault=NAME   注入一種故障，NAME 見 --help
 *       ./boot_sim --test         執行自我檢查
 */
#include <stdarg.h>
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

/* ---- 記憶體配置（相當於 linker script 的 MEMORY 區塊） ---- */
#define FLASH_BASE 0x00000000u
#define FLASH_SIZE 0x00001000u /* 4 KiB，教學用的小容量 */
#define SRAM_BASE 0x20000000u
#define SRAM_SIZE 0x00000800u /* 2 KiB */

static uint8_t flash[FLASH_SIZE];
static uint8_t sram[SRAM_SIZE];

/* ---- linker 算出的區段地址（相當於 map file 裡的內容） ---- */
typedef struct {
    uint32_t isr_vector;            /* 向量表，位於 Flash 開頭 */
    uint32_t text_start, text_end;  /* 程式碼 */
    uint32_t rodata_start, rodata_end;
    uint32_t data_lma;              /* .data 初值存放處（Flash） */
    uint32_t data_start, data_end;  /* .data 執行時位置（SRAM） */
    uint32_t bss_start, bss_end;
    uint32_t noinit_start, noinit_end;
    uint32_t stack_top;             /* 初始 SP，常見名稱如 _estack */
} layout_t;

/* 程式中的「全域變數」在 SRAM 內的地址；由 link() 填入 */
typedef struct {
    uint32_t g_sample_period_ms; /* .data，初值 10 */
    uint32_t g_uart_baud;        /* .data，初值 115200 */
    uint32_t g_sample_count;     /* .bss */
    uint32_t g_rx_errors;        /* .bss */
    uint32_t n_magic;            /* .noinit */
    uint32_t n_warm_resets;      /* .noinit */
    uint32_t n_check;            /* .noinit，n_warm_resets 的反碼 */
    uint32_t k_fw_name;          /* .rodata，位於 Flash */
} symbols_t;

/* 假的程式碼地址：模擬中用來代表函式入口 */
enum { CODE_RESET = 0x0100u, CODE_DEFAULT = 0x0180u, CODE_MAIN = 0x0200u };

#define VECTOR_COUNT 16u
#define NOINIT_MAGIC 0x52535431u /* 'RST1' */

typedef enum {
    FAULT_NONE,
    FAULT_SKIP_DATA_COPY, /* 啟動碼忘了搬 .data */
    FAULT_SKIP_BSS_ZERO,  /* 啟動碼忘了清 .bss */
    FAULT_WRONG_LMA,      /* 從錯誤的 Flash 地址搬 .data */
    FAULT_THUMB_BIT,      /* reset 向量 bit0 = 0 */
    FAULT_SP_OUTSIDE_RAM, /* 初始 SP 指到 SRAM 以外 */
    FAULT_COUNT
} fault_t;

static const char *const fault_names[FAULT_COUNT] = {
    "none", "skip-data-copy", "skip-bss-zero", "wrong-lma", "thumb-bit", "sp-outside-ram",
};

typedef struct {
    int reached_main;
    int faulted;
    char fault_msg[128];
    uint32_t msp, pc;
    uint32_t period_seen_by_systeminit; /* SystemInit 執行時看到的 .data 內容 */
    uint32_t period, baud;              /* main 看到的 .data */
    uint32_t count_at_main;             /* main 開始時的 .bss */
    uint32_t warm_resets;               /* .noinit 中的計數 */
    char fw_name[16];
} boot_result_t;

static layout_t L;
static symbols_t S;
static int g_trace = 1;
static boot_result_t *g_res;

static void trace(const char *fmt, ...)
{
    if (!g_trace) return;
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
}

static void raise_fault(const char *msg)
{
    if (!g_res->faulted) {
        g_res->faulted = 1;
        snprintf(g_res->fault_msg, sizeof g_res->fault_msg, "%s", msg);
    }
    trace("  !! FAULT: %s\n", msg);
}

/* ---- 匯流排：把地址對應到 flash[] 或 sram[]，超出範圍就是 fault ---- */
static uint8_t *bus(uint32_t addr, uint32_t len, int write)
{
    uint32_t off = addr - FLASH_BASE; /* unsigned 相減：addr 低於起點時會繞成很大的值 */
    if (off < FLASH_SIZE && len <= FLASH_SIZE - off) {
        if (write) {
            raise_fault("寫入 Flash：執行期 Flash 不能用一般 store 改寫");
            return NULL;
        }
        return &flash[off];
    }
    off = addr - SRAM_BASE;
    if (off < SRAM_SIZE && len <= SRAM_SIZE - off)
        return &sram[off];
    char msg[96];
    snprintf(msg, sizeof msg, "存取不存在的地址 0x%08X（真實晶片的反應依匯流排設計而定）",
             (unsigned)addr);
    raise_fault(msg);
    return NULL;
}

static uint32_t rd32(uint32_t addr)
{
    uint8_t *p = bus(addr, 4, 0);
    if (!p) return 0;
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static void wr32(uint32_t addr, uint32_t v)
{
    uint8_t *p = bus(addr, 4, 1);
    if (!p) return;
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

/* 燒錄工具寫 Flash 用的路徑；CPU 執行期不走這裡 */
static void flash_program32(uint32_t addr, uint32_t v)
{
    uint32_t off = addr - FLASH_BASE;
    flash[off] = (uint8_t)v;
    flash[off + 1] = (uint8_t)(v >> 8);
    flash[off + 2] = (uint8_t)(v >> 16);
    flash[off + 3] = (uint8_t)(v >> 24);
}

static uint32_t align_up(uint32_t v, uint32_t a) { return (v + a - 1u) & ~(a - 1u); }

/* ---- 「linker」：排區段、算符號，並產生要燒進 Flash 的映像 ---- */
static void link(fault_t f)
{
    memset(flash, 0xFF, sizeof flash); /* 抹除後的 Flash 常讀成 0xFF */

    L.isr_vector = FLASH_BASE;
    L.text_start = FLASH_BASE + VECTOR_COUNT * 4u;
    L.text_end = FLASH_BASE + 0x0300u;
    L.rodata_start = L.text_end;
    static const char fw_name[] = "logger-v1";
    L.rodata_end = align_up(L.rodata_start + (uint32_t)sizeof fw_name, 4);

    L.data_start = SRAM_BASE;                /* VMA：執行時在 SRAM */
    L.data_end = L.data_start + 2u * 4u;     /* 兩個 uint32_t */
    L.data_lma = L.rodata_end;               /* LMA：初值緊接在 .rodata 後面 */
    L.bss_start = L.data_end;
    L.bss_end = L.bss_start + 2u * 4u;
    L.noinit_start = L.bss_end;
    L.noinit_end = L.noinit_start + 3u * 4u;
    L.stack_top = SRAM_BASE + SRAM_SIZE;     /* stack 從 SRAM 尾端往下長 */

    S.g_sample_period_ms = L.data_start;
    S.g_uart_baud = L.data_start + 4u;
    S.g_sample_count = L.bss_start;
    S.g_rx_errors = L.bss_start + 4u;
    S.n_magic = L.noinit_start;
    S.n_warm_resets = L.noinit_start + 4u;
    S.n_check = L.noinit_start + 8u;
    S.k_fw_name = L.rodata_start;

    /* 向量表：word 0 = 初始 SP，word 1 = reset handler，其餘先指向預設 handler。
     * 每個 handler 地址的 bit0 必須是 1，表示 Thumb 狀態。 */
    uint32_t sp = (f == FAULT_SP_OUTSIDE_RAM) ? SRAM_BASE + SRAM_SIZE + 0x100u : L.stack_top;
    uint32_t reset = (f == FAULT_THUMB_BIT) ? CODE_RESET : (CODE_RESET | 1u);
    flash_program32(L.isr_vector + 0u, sp);
    flash_program32(L.isr_vector + 4u, reset);
    for (uint32_t i = 2; i < VECTOR_COUNT; i++)
        flash_program32(L.isr_vector + 4u * i, CODE_DEFAULT | 1u);

    memcpy(&flash[L.rodata_start - FLASH_BASE], fw_name, sizeof fw_name);
    flash_program32(L.data_lma + 0u, 10u);     /* g_sample_period_ms 的初值 */
    flash_program32(L.data_lma + 4u, 115200u); /* g_uart_baud 的初值 */
}

static void print_map(void)
{
    printf("== 模擬 map（VMA 是執行地址，LMA 是存放地址） ==\n");
    printf("  .isr_vector  0x%08X  %3u bytes  Flash\n", (unsigned)L.isr_vector, VECTOR_COUNT * 4u);
    printf("  .text        0x%08X  %3u bytes  Flash\n", (unsigned)L.text_start,
           (unsigned)(L.text_end - L.text_start));
    printf("  .rodata      0x%08X  %3u bytes  Flash   k_fw_name\n", (unsigned)L.rodata_start,
           (unsigned)(L.rodata_end - L.rodata_start));
    printf("  .data   VMA  0x%08X  %3u bytes  SRAM    g_sample_period_ms, g_uart_baud\n",
           (unsigned)L.data_start, (unsigned)(L.data_end - L.data_start));
    printf("          LMA  0x%08X              Flash   初值存放處\n", (unsigned)L.data_lma);
    printf("  .bss         0x%08X  %3u bytes  SRAM    g_sample_count, g_rx_errors\n",
           (unsigned)L.bss_start, (unsigned)(L.bss_end - L.bss_start));
    printf("  .noinit      0x%08X  %3u bytes  SRAM    n_magic, n_warm_resets, n_check\n",
           (unsigned)L.noinit_start, (unsigned)(L.noinit_end - L.noinit_start));
    printf("  _estack      0x%08X                     初始 SP\n\n", (unsigned)L.stack_top);
}

/* ---- 上電：SRAM 內容不可預期；用固定種子的亂數代表 ---- */
static void power_on_sram(uint32_t seed)
{
    for (uint32_t i = 0; i < SRAM_SIZE; i++) {
        seed = seed * 1664525u + 1013904223u;
        sram[i] = (uint8_t)(seed >> 24);
    }
}

/* ---- 模擬中的韌體 ---- */
static void system_init(void)
{
    /* CMSIS 風格的啟動碼在 C runtime 初始化之前呼叫 SystemInit。
     * 此時 .data 還沒搬、.bss 還沒清，讀全域變數只會讀到 SRAM 原本的內容。 */
    g_res->period_seen_by_systeminit = rd32(S.g_sample_period_ms);
    trace("  SystemInit：設定時脈；此時 g_sample_period_ms = 0x%08X（本次尚未初始化，是 SRAM 原有內容）\n",
          (unsigned)g_res->period_seen_by_systeminit);
}

static void firmware_main(void)
{
    g_res->reached_main = 1;
    g_res->period = rd32(S.g_sample_period_ms);
    g_res->baud = rd32(S.g_uart_baud);
    g_res->count_at_main = rd32(S.g_sample_count);
    for (int i = 0; i < 16 && i < (int)sizeof g_res->fw_name - 1; i++) {
        uint8_t *p = bus(S.k_fw_name + (uint32_t)i, 1, 0);
        if (!p || *p == 0) break;
        g_res->fw_name[i] = (char)*p;
    }

    /* .noinit：只有 magic 與檢查碼都對，才相信上次留下來的值 */
    uint32_t magic = rd32(S.n_magic);
    uint32_t n = rd32(S.n_warm_resets);
    uint32_t chk = rd32(S.n_check);
    if (magic == NOINIT_MAGIC && chk == ~n) {
        n++;
    } else {
        n = 0;
        wr32(S.n_magic, NOINIT_MAGIC);
    }
    wr32(S.n_warm_resets, n);
    wr32(S.n_check, ~n);
    g_res->warm_resets = n;

    trace("  main：period=%u ms, baud=%u, sample_count 起始=%u, fw=\"%s\", warm resets=%u\n",
          (unsigned)g_res->period, (unsigned)g_res->baud, (unsigned)g_res->count_at_main,
          g_res->fw_name, (unsigned)n);

    /* 模擬工作：取樣 5 次。warm reset 後若 .bss 沒清，這個值會殘留 */
    for (int i = 0; i < 5; i++)
        wr32(S.g_sample_count, rd32(S.g_sample_count) + 1u);
}

static void reset_handler(uint32_t msp, fault_t f)
{
    /* 啟動碼呼叫函式時會用到 stack；SP 不在 SRAM 內，第一次 push 就會出錯 */
    wr32(msp - 4u, 0xFFFFFFFFu);
    if (g_res->faulted) return;

    system_init();

    if (f != FAULT_SKIP_DATA_COPY) {
        uint32_t src = L.data_lma + (f == FAULT_WRONG_LMA ? 4u : 0u);
        trace("  Reset_Handler：複製 .data  0x%08X -> 0x%08X，%u bytes\n", (unsigned)src,
              (unsigned)L.data_start, (unsigned)(L.data_end - L.data_start));
        for (uint32_t d = L.data_start; d < L.data_end; d += 4u, src += 4u)
            wr32(d, rd32(src));
    } else {
        trace("  Reset_Handler：（故障注入）略過 .data 複製\n");
    }

    if (f != FAULT_SKIP_BSS_ZERO) {
        trace("  Reset_Handler：清零 .bss   0x%08X..0x%08X\n", (unsigned)L.bss_start,
              (unsigned)L.bss_end);
        for (uint32_t d = L.bss_start; d < L.bss_end; d += 4u)
            wr32(d, 0u);
    } else {
        trace("  Reset_Handler：（故障注入）略過 .bss 清零\n");
    }
    trace("  Reset_Handler：.noinit 不處理，呼叫 main\n");

    firmware_main();
    trace("  main 返回：啟動碼通常在此進入無窮迴圈\n");
}

/* ---- 「硬體」reset 流程 ---- */
typedef enum { RESET_POWER_ON, RESET_WARM } reset_kind_t;

static void cpu_reset(reset_kind_t kind, fault_t f, uint32_t seed, boot_result_t *res)
{
    memset(res, 0, sizeof *res);
    g_res = res;
    if (kind == RESET_POWER_ON) power_on_sram(seed);
    /* warm reset：SRAM 通常保留原內容，但是否保證保留要查晶片手冊 */

    trace("[%s reset]\n", kind == RESET_POWER_ON ? "power-on" : "warm");
    res->msp = rd32(L.isr_vector + 0u); /* 硬體：從 offset 0 載入 MSP */
    res->pc = rd32(L.isr_vector + 4u);  /* 硬體：從 offset 4 載入 PC */
    trace("  硬體：MSP <- [0x%08X] = 0x%08X\n", (unsigned)L.isr_vector, (unsigned)res->msp);
    trace("  硬體：PC  <- [0x%08X] = 0x%08X\n", (unsigned)(L.isr_vector + 4u), (unsigned)res->pc);

    if ((res->pc & 1u) == 0u) {
        raise_fault("reset 向量 bit0 = 0：EPSR.T 被清除，第一條指令就會進入 fault");
        return;
    }
    switch (res->pc & ~1u) {
    case CODE_RESET:
        reset_handler(res->msp, f);
        break;
    default:
        raise_fault("reset 向量指向沒有程式的地址");
        break;
    }
}

/* ---- 自我檢查 ---- */
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
    boot_result_t r;
    g_trace = 0;

    printf("正常上電\n");
    link(FAULT_NONE);
    cpu_reset(RESET_POWER_ON, FAULT_NONE, 1u, &r);
    CHECK(r.reached_main && !r.faulted);
    CHECK(r.msp == SRAM_BASE + SRAM_SIZE);
    CHECK(r.pc == (CODE_RESET | 1u));
    CHECK(r.period == 10u && r.baud == 115200u);
    CHECK(r.count_at_main == 0u);
    CHECK(strcmp(r.fw_name, "logger-v1") == 0);
    CHECK(r.period_seen_by_systeminit != 10u); /* SystemInit 早於 .data 複製 */
    CHECK(r.warm_resets == 0u);

    printf("warm reset 三次：.noinit 保留，.bss 每次重新清零\n");
    for (int i = 0; i < 3; i++) cpu_reset(RESET_WARM, FAULT_NONE, 0u, &r);
    CHECK(r.warm_resets == 3u);
    CHECK(r.count_at_main == 0u);
    CHECK(r.period_seen_by_systeminit == 10u); /* 殘留值：看起來對，其實尚未初始化 */

    printf("再次上電：.noinit 內容不可信，檢查碼讓計數歸零\n");
    cpu_reset(RESET_POWER_ON, FAULT_NONE, 7u, &r);
    CHECK(r.warm_resets == 0u);

    printf("故障：略過 .bss 清零，warm reset 後看到上一輪的計數\n");
    cpu_reset(RESET_WARM, FAULT_SKIP_BSS_ZERO, 0u, &r);
    CHECK(r.reached_main && r.count_at_main == 5u);

    printf("故障：略過 .data 複製，上電後讀到不可預期的值\n");
    cpu_reset(RESET_POWER_ON, FAULT_SKIP_DATA_COPY, 3u, &r);
    CHECK(r.reached_main && r.period != 10u);

    printf("故障：LMA 錯一個 word，數值整體錯位\n");
    cpu_reset(RESET_POWER_ON, FAULT_WRONG_LMA, 1u, &r);
    CHECK(r.reached_main && r.period == 115200u);

    printf("故障：reset 向量 bit0 = 0\n");
    link(FAULT_THUMB_BIT);
    cpu_reset(RESET_POWER_ON, FAULT_NONE, 1u, &r);
    CHECK(r.faulted && !r.reached_main);

    printf("故障：初始 SP 在 SRAM 之外\n");
    link(FAULT_SP_OUTSIDE_RAM);
    cpu_reset(RESET_POWER_ON, FAULT_NONE, 1u, &r);
    CHECK(r.faulted && !r.reached_main);

    printf(g_fail ? "\n%d 項失敗\n" : "\n全部通過\n", g_fail);
    return g_fail ? 1 : 0;
}

static void usage(void)
{
    printf("用法：boot_sim [--test] [--fault=NAME]\n故障名稱：");
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

    boot_result_t r;
    link(f);
    print_map();
    cpu_reset(RESET_POWER_ON, f, 1u, &r);
    printf("\n");
    cpu_reset(RESET_WARM, f, 0u, &r);
    return 0;
}

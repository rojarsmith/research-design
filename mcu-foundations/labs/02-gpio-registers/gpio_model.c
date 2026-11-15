/*
 * gpio_model.c：用一個虛構的 GPIO 周邊，練習「從 API 追到暫存器，再追到接腳」。
 *
 * 暫存器配置是教學用的虛構設計，不屬於任何廠商，但它包含多數 MCU 都有的機制：
 *   CLK.EN      時脈閘控，bit0 = GPIOA；沒開時脈時，本模型的周邊讀到 0、忽略寫入
 *               （真實晶片可能忽略、回傳 0 或產生 bus fault，要查手冊）
 *   GPIOA.MODE  每支接腳 2 bits：00 輸入、01 輸出、10 替代功能、11 類比
 *   GPIOA.PULL  每支接腳 2 bits：00 無、01 上拉、10 下拉
 *   GPIOA.OUT   輸出資料暫存器（可讀可寫）
 *   GPIOA.IN    輸入資料暫存器（唯讀，反映接腳實際電位）
 *   GPIOA.SETCLR 只寫：bit n 設定 OUT 的 bit n；bit (n+16) 清除 OUT 的 bit n
 *   GPIOA.AF    PA8 到 PA15 每支 4 bits，選擇替代功能編號
 *
 * 板子接線（同樣是虛構的）：
 *   PA5：LED 與電阻串接到 VDD，接腳輸出低電位時電流流入接腳，LED 亮（低電位有效）
 *   PA6：第二顆 LED，接法相同，由「中斷」閃爍
 *   PA9：替代功能 1 = UART TX，閒置時由 UART 驅動成高電位
 *   PA13：按鈕接到 GND，按下時接腳被拉到低電位；沒有上拉時放開就是浮接
 *
 * 建置：gcc -std=c11 -Wall -Wextra -Werror -O2 -o gpio_model gpio_model.c
 * 執行：./gpio_model --trace   顯示 HAL 呼叫產生的每一次暫存器存取
 *       ./gpio_model --test    執行自我檢查
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

/* ---------------- 硬體模型 ---------------- */
enum { REG_CLK_EN, REG_MODE, REG_PULL, REG_OUT, REG_IN, REG_SETCLR, REG_AF, REG_COUNT };
static const char *const reg_names[REG_COUNT] = {
    "CLK.EN", "GPIOA.MODE", "GPIOA.PULL", "GPIOA.OUT", "GPIOA.IN", "GPIOA.SETCLR", "GPIOA.AF",
};

enum { MODE_INPUT = 0u, MODE_OUTPUT = 1u, MODE_AF = 2u, MODE_ANALOG = 3u };
enum { PULL_NONE = 0u, PULL_UP = 1u, PULL_DOWN = 2u };
enum { PIN_LED = 5, PIN_LED2 = 6, PIN_UART_TX = 9, PIN_BUTTON = 13 };

typedef struct {
    uint32_t clk_en;
    uint32_t mode, pull, out, af;
    int button_pressed;    /* 外界狀態：按鈕是否按下 */
    uint32_t noise;        /* 浮接接腳的讀值來源 */
} hw_t;

static hw_t hw;
static int g_trace;

static uint32_t field_get(uint32_t reg, unsigned pin, unsigned width)
{
    return (reg >> (pin * width)) & ((1u << width) - 1u);
}

/* 依接線與暫存器狀態算出某支接腳的電位：1 高、0 低、-1 浮接 */
static int pin_level(unsigned pin)
{
    if (!(hw.clk_en & 1u)) return pin == PIN_LED || pin == PIN_LED2 ? 1 : -1; /* 重置後為輸入 */
    uint32_t mode = field_get(hw.mode, pin, 2);
    if (mode == MODE_OUTPUT) return (int)((hw.out >> pin) & 1u);
    if (mode == MODE_AF) {
        if (pin == PIN_UART_TX && field_get(hw.af, pin - 8u, 4) == 1u) return 1; /* UART 閒置 */
        return -1;
    }
    if (mode == MODE_ANALOG) return -1;
    /* 輸入模式：由外部電路與上下拉決定 */
    if (pin == PIN_LED || pin == PIN_LED2) return 1; /* 經 LED 與電阻被拉向 VDD */
    if (pin == PIN_BUTTON && hw.button_pressed) return 0;
    uint32_t pull = field_get(hw.pull, pin, 2);
    if (pull == PULL_UP) return 1;
    if (pull == PULL_DOWN) return 0;
    return -1;
}

static int led_is_on(unsigned pin) { return pin_level(pin) == 0; }

static uint32_t read_in_register(void)
{
    uint32_t v = 0;
    for (unsigned pin = 0; pin < 16; pin++) {
        int lv = pin_level(pin);
        if (lv < 0) { /* 浮接：每次讀到的值不可預期 */
            hw.noise = hw.noise * 1103515245u + 12345u;
            lv = (int)((hw.noise >> 16) & 1u);
        }
        v |= (uint32_t)lv << pin;
    }
    return v;
}

static uint32_t reg_read(int r)
{
    uint32_t v = 0;
    if (r == REG_CLK_EN) {
        v = hw.clk_en;
    } else if (hw.clk_en & 1u) {
        switch (r) {
        case REG_MODE: v = hw.mode; break;
        case REG_PULL: v = hw.pull; break;
        case REG_OUT: v = hw.out; break;
        case REG_IN: v = read_in_register(); break;
        case REG_AF: v = hw.af; break;
        default: v = 0; break; /* SETCLR 只寫，讀到 0 */
        }
    }
    if (g_trace) printf("    [R] %-12s -> 0x%08X\n", reg_names[r], (unsigned)v);
    return v;
}

static void reg_write(int r, uint32_t v)
{
    if (g_trace)
        printf("    [W] %-12s <- 0x%08X%s\n", reg_names[r], (unsigned)v,
               r != REG_CLK_EN && !(hw.clk_en & 1u) ? "  （沒有時脈，寫入被忽略）" : "");
    if (r == REG_CLK_EN) {
        hw.clk_en = v;
        return;
    }
    if (!(hw.clk_en & 1u)) return;
    switch (r) {
    case REG_MODE: hw.mode = v; break;
    case REG_PULL: hw.pull = v; break;
    case REG_OUT: hw.out = v & 0xFFFFu; break;
    case REG_AF: hw.af = v; break;
    case REG_SETCLR: /* 一次寫入完成，不需要先讀：不受中斷打斷影響 */
        hw.out |= v & 0xFFFFu;
        hw.out &= ~(v >> 16);
        break;
    default: break; /* IN 唯讀 */
    }
}

static void hw_reset(void)
{
    memset(&hw, 0, sizeof hw);
    hw.noise = 12345u;
}

/* ---------------- 模擬中斷：在主程式的讀與寫之間插入 ISR ---------------- */
static void (*g_pending_isr)(void);

static void preempt_point(void)
{
    if (g_pending_isr) {
        void (*isr)(void) = g_pending_isr;
        g_pending_isr = NULL;
        isr(); /* 單核心：ISR 執行完才回到主程式 */
    }
}

static void isr_toggle_led2(void)
{
    if (g_trace) printf("    -- 進入 ISR --\n");
    uint32_t v = reg_read(REG_OUT);
    reg_write(REG_OUT, v ^ (1u << PIN_LED2));
}

/* ---------------- 韌體：像 HAL 一樣的函式 ---------------- */
static void gpio_set_mode_buggy(unsigned pin, uint32_t mode)
{
    reg_write(REG_MODE, reg_read(REG_MODE) | (mode << (pin * 2u))); /* 忘了先清掉舊值 */
}

static void gpio_set_mode(unsigned pin, uint32_t mode)
{
    uint32_t v = reg_read(REG_MODE);
    v &= ~(3u << (pin * 2u));
    v |= (mode & 3u) << (pin * 2u);
    reg_write(REG_MODE, v);
}

static void gpio_set_pull(unsigned pin, uint32_t pull)
{
    uint32_t v = reg_read(REG_PULL);
    v &= ~(3u << (pin * 2u));
    v |= (pull & 3u) << (pin * 2u);
    reg_write(REG_PULL, v);
}

/* 寫法一：對 OUT 做讀-改-寫。讀和寫之間若被中斷打斷，ISR 對 OUT 的修改會被覆蓋 */
static void gpio_write_rmw(unsigned pin, int high)
{
    uint32_t v = reg_read(REG_OUT);
    preempt_point();
    if (high)
        v |= 1u << pin;
    else
        v &= ~(1u << pin);
    reg_write(REG_OUT, v);
}

/* 寫法二：用 SETCLR 一次寫完 */
static void gpio_write(unsigned pin, int high)
{
    preempt_point();
    reg_write(REG_SETCLR, high ? 1u << pin : 1u << (pin + 16u));
}

/* 應用層看到的是「LED 亮/滅」，低電位有效的細節留在板級這一層 */
static void board_led_init(void)
{
    reg_write(REG_CLK_EN, reg_read(REG_CLK_EN) | 1u);
    gpio_write(PIN_LED, 1); /* 先讓輸出鎖存為高電位，切成輸出時 LED 才不會閃一下 */
    gpio_set_mode(PIN_LED, MODE_OUTPUT);
}
static void board_led_set(int on) { gpio_write(PIN_LED, on ? 0 : 1); }

static int board_button_pressed(void) { return ((reg_read(REG_IN) >> PIN_BUTTON) & 1u) == 0u; }

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

static int lost_updates_with(void (*write_fn)(unsigned, int), int iterations)
{
    hw_reset();
    reg_write(REG_CLK_EN, 1u);
    gpio_set_mode(PIN_LED, MODE_OUTPUT);
    gpio_set_mode(PIN_LED2, MODE_OUTPUT);
    int lost = 0;
    for (int i = 0; i < iterations; i++) {
        uint32_t led2_before = (hw.out >> PIN_LED2) & 1u;
        if (i % 3 == 0) g_pending_isr = isr_toggle_led2; /* 每三次寫入有一次被打斷 */
        int isr_ran = g_pending_isr != NULL;
        write_fn(PIN_LED, i & 1);
        uint32_t led2_after = (hw.out >> PIN_LED2) & 1u;
        if (isr_ran && led2_after == led2_before) lost++; /* ISR 的切換不見了 */
    }
    return lost;
}

static int run_tests(void)
{
    printf("時脈沒開：寫入無效，LED 不亮\n");
    hw_reset();
    gpio_set_mode(PIN_LED, MODE_OUTPUT);
    gpio_write(PIN_LED, 0);
    CHECK(reg_read(REG_MODE) == 0u);
    CHECK(!led_is_on(PIN_LED));

    printf("開時脈後再設定：LED 低電位有效\n");
    board_led_init();
    CHECK(!led_is_on(PIN_LED)); /* 初始化完成時應該是滅的 */
    board_led_set(1);
    CHECK(led_is_on(PIN_LED) && ((hw.out >> PIN_LED) & 1u) == 0u);
    board_led_set(0);
    CHECK(!led_is_on(PIN_LED) && ((hw.out >> PIN_LED) & 1u) == 1u);

    printf("輸入模式：OUT 的值不會出現在接腳上\n");
    gpio_set_mode(PIN_LED, MODE_INPUT);
    gpio_write(PIN_LED, 0);
    CHECK(!led_is_on(PIN_LED));
    CHECK(((reg_read(REG_IN) >> PIN_LED) & 1u) == 1u); /* 經 LED 被拉向 VDD */

    printf("模式欄位沒有先清除：輸出(01) 改成替代功能(10) 變成 11\n");
    hw_reset();
    reg_write(REG_CLK_EN, 1u);
    gpio_set_mode(PIN_UART_TX, MODE_OUTPUT);
    gpio_set_mode_buggy(PIN_UART_TX, MODE_AF);
    CHECK(field_get(hw.mode, PIN_UART_TX, 2) == MODE_ANALOG);
    gpio_set_mode(PIN_UART_TX, MODE_AF);
    CHECK(field_get(hw.mode, PIN_UART_TX, 2) == MODE_AF);
    CHECK(field_get(hw.mode, PIN_UART_TX - 1u, 2) == 0u && field_get(hw.mode, PIN_UART_TX + 1u, 2) == 0u);

    printf("替代功能：接腳由 UART 驅動，寫 OUT 沒有效果\n");
    reg_write(REG_AF, 1u << ((PIN_UART_TX - 8u) * 4u));
    gpio_write(PIN_UART_TX, 0);
    CHECK(pin_level(PIN_UART_TX) == 1);

    printf("按鈕：沒有上拉時放開是浮接，讀值不可預期\n");
    hw.button_pressed = 0;
    int ones = 0;
    for (int i = 0; i < 100; i++) ones += !board_button_pressed();
    CHECK(ones > 0 && ones < 100);
    gpio_set_pull(PIN_BUTTON, PULL_UP);
    ones = 0;
    for (int i = 0; i < 100; i++) ones += !board_button_pressed();
    CHECK(ones == 100);
    hw.button_pressed = 1;
    CHECK(board_button_pressed());

    printf("讀-改-寫被中斷打斷\n");
    int lost_rmw = lost_updates_with(gpio_write_rmw, 300);
    int lost_setclr = lost_updates_with(gpio_write, 300);
    printf("  RMW 遺失 %d 次 ISR 更新；SETCLR 遺失 %d 次\n", lost_rmw, lost_setclr);
    CHECK(lost_rmw == 100);
    CHECK(lost_setclr == 0);

    printf(g_fail ? "\n%d 項失敗\n" : "\n全部通過\n", g_fail);
    return g_fail ? 1 : 0;
}

static void run_trace(void)
{
    g_trace = 1;
    hw_reset();
    printf("board_led_init()：開時脈、先鎖存高電位、再切成輸出\n");
    board_led_init();
    printf("board_led_set(1)：低電位有效，所以是「清除」PA5\n");
    board_led_set(1);
    printf("  LED 狀態：%s\n", led_is_on(PIN_LED) ? "亮" : "滅");
    printf("gpio_write_rmw(PA5, 1) 途中被 ISR 打斷，ISR 切換 PA6\n");
    gpio_set_mode(PIN_LED2, MODE_OUTPUT);
    g_pending_isr = isr_toggle_led2;
    gpio_write_rmw(PIN_LED, 1);
    printf("  PA6 輸出鎖存 = %u（ISR 寫成 1，又被主程式覆寫回 0）\n",
           (unsigned)((hw.out >> PIN_LED2) & 1u));
}

int main(int argc, char **argv)
{
    console_utf8();
    if (argc > 1 && strcmp(argv[1], "--test") == 0) return run_tests();
    if (argc > 1 && strcmp(argv[1], "--trace") == 0) {
        run_trace();
        return 0;
    }
    printf("用法：gpio_model --trace | --test\n");
    return argc > 1 ? 2 : 0;
}

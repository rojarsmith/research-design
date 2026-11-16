/*
 * sim_a.c：平台 A 的硬體模型與測試台。真實的板子上不存在這個檔案。
 *
 * 每 1 ms 真實時間推進一步：
 *   1. 依目前的系統時脈算出這 1 ms 有幾次 SysTick，逐次呼叫 SysTick_Handler()。
 *   2. 電腦送來的 byte 依線路速率到達 UART：設 RXNE、觸發中斷；上一個還沒被讀走就設 ORE。
 *      裝置的 baud（周邊時脈 ÷ BAUDDIV）和電腦差超過 3%，收到的 byte 是錯的，並設 FE。
 *   3. 主迴圈執行 MAIN_LOOPS_PER_MS 圈 board_main_once()（被要求卡住、或 CPU 困在中斷裡時不執行）。
 *      感測器的轉換時間比一圈主迴圈短：開始之後，下一圈就讀得到 DONE；拔掉時永遠不完成。
 *   4. UART 依線路速率把 DATA 裡的 byte 送給電腦，每送完一個就設 TXE、觸發中斷。
 * 中斷是 level 觸發：條件還成立就再進一次。同一次評估裡進入超過 1000 次，
 * 視為中斷風暴，這 1 ms 主迴圈得不到執行時間。
 */
#include <stdio.h>
#include <string.h>

#include "sim.h"
#include "vendor_a.h"

#define HSI_HZ 8000000u
#define HOST_BAUD 115200u
#define LINE_BYTES_PER_S 11520u /* 8N1：每 byte 10 bits */
#define STORM_LIMIT 1000u
#define MAIN_LOOPS_PER_MS 4u /* 真實的主迴圈每毫秒跑很多圈；模型取 4 圈 */

va_clk_t va_sim_clk;
va_systick_t va_sim_systick;
va_uart_t va_sim_uart1;
va_sens_t va_sim_sens;

static struct {
    uint32_t real_ms;
    host_sink_t sink;
    void *sink_ctx;
    uint8_t to_dev[4096];
    uint32_t to_dev_head, to_dev_n;
    uint32_t rx_credit, tx_credit;
    uint32_t block_until;
    int sensor_connected;
    /* 硬體內部狀態 */
    uint32_t pll_reads;     /* PLLON 之後讀了幾次 STAT */
    uint64_t systick_cycles;
    uint8_t rx_data;
    uint8_t tx_hold;
    int tx_full;
    int sens_busy;
    uint32_t sens_n;
    int16_t sens_value;
    int nvic_uart, masked, in_isr, storm_now;
} g;

/* 跨多次 sim_reset 累積的統計 */
static struct {
    uint64_t systick_irqs, uart_irqs;
    uint32_t max_uart_irqs_per_ms, overruns, framing, storm_ms, resets;
    uint32_t hclk_last, bauddiv_last;
} st;
static uint32_t g_uart_irqs_this_ms;

static uint32_t hclk(void)
{
    if (va_sim_clk.SEL == 1u && (va_sim_clk.STAT & VA_CLK_PLLRDY)) return HSI_HZ * va_sim_clk.PLLMUL;
    return HSI_HZ;
}

/* 裝置的實際 baud 和電腦差太多：雙方各自取樣，byte 會錯 */
static int baud_mismatch(void)
{
    if (!va_sim_uart1.BAUDDIV) return 1;
    uint32_t actual = hclk() / va_sim_uart1.BAUDDIV;
    uint32_t diff = actual > HOST_BAUD ? actual - HOST_BAUD : HOST_BAUD - actual;
    return diff * 100u > HOST_BAUD * 3u;
}

static int uart_on(void) { return (va_sim_clk.GATE & VA_CLK_GATE_UART1) && (va_sim_uart1.CTRL & VA_UART_EN); }

static int uart_irq_line(void)
{
    uint32_t c = va_sim_uart1.CTRL, s = va_sim_uart1.STAT;
    if (!uart_on()) return 0;
    return ((s & VA_UART_RXNE) && (c & VA_UART_RXIE)) || ((s & VA_UART_TXE) && (c & VA_UART_TXIE)) ||
           ((s & (VA_UART_ORE | VA_UART_FE)) && (c & VA_UART_RXIE));
}

/* NVIC：條件成立就進 handler，返回後再看一次 */
static void irq_eval(void)
{
    if (g.in_isr || g.masked || !g.nvic_uart) return;
    uint32_t n = 0;
    while (uart_irq_line()) {
        if (++n > STORM_LIMIT) {
            g.storm_now = 1;
            return;
        }
        g.in_isr = 1;
        VA_UART1_IRQHandler();
        g.in_isr = 0;
        st.uart_irqs++;
        g_uart_irqs_this_ms++;
    }
}

/* 用相等比較判斷是哪個周邊：不同物件的指標不能用大小比較 */
static int is_uart_reg(const volatile uint32_t *r)
{
    return r == &va_sim_uart1.CTRL || r == &va_sim_uart1.STAT || r == &va_sim_uart1.DATA ||
           r == &va_sim_uart1.BAUDDIV;
}

static int is_sens_reg(const volatile uint32_t *r)
{
    return r == &va_sim_sens.CTRL || r == &va_sim_sens.STAT || r == &va_sim_sens.DATA;
}

uint32_t va_bus_read(volatile uint32_t *reg)
{
    if (reg == &va_sim_clk.STAT) {
        if ((va_sim_clk.CTRL & VA_CLK_PLLON) && ++g.pll_reads >= 3u) va_sim_clk.STAT |= VA_CLK_PLLRDY;
        return va_sim_clk.STAT;
    }
    if (reg == &va_sim_uart1.DATA) {
        if (!(va_sim_clk.GATE & VA_CLK_GATE_UART1)) return 0u;
        va_sim_uart1.STAT &= ~VA_UART_RXNE;
        return g.rx_data;
    }
    if (reg == &va_sim_sens.DATA) {
        if (!(va_sim_clk.GATE & VA_CLK_GATE_SENS)) return 0u;
        va_sim_sens.STAT &= ~VA_SENS_DONE;
        return (uint16_t)g.sens_value;
    }
    if ((is_uart_reg(reg) && !(va_sim_clk.GATE & VA_CLK_GATE_UART1)) ||
        (is_sens_reg(reg) && !(va_sim_clk.GATE & VA_CLK_GATE_SENS)))
        return 0u; /* 沒開時脈：讀到 0 */
    return *reg;
}

void va_bus_write(volatile uint32_t *reg, uint32_t value)
{
    if (is_uart_reg(reg)) {
        if (!(va_sim_clk.GATE & VA_CLK_GATE_UART1)) return; /* 沒開時脈：寫入無效 */
        if (reg == &va_sim_uart1.STAT) {
            va_sim_uart1.STAT &= ~(value & (VA_UART_ORE | VA_UART_FE)); /* 寫 1 清除 */
        } else if (reg == &va_sim_uart1.DATA) {
            if (!g.tx_full) {
                g.tx_hold = (uint8_t)value;
                g.tx_full = 1;
                va_sim_uart1.STAT &= ~VA_UART_TXE;
            }
        } else {
            *reg = value;
        }
        irq_eval(); /* 例如剛打開 TXIE、而 TXE 本來就成立：立刻進中斷 */
        return;
    }
    if (is_sens_reg(reg)) {
        if (!(va_sim_clk.GATE & VA_CLK_GATE_SENS)) return;
        if (reg == &va_sim_sens.CTRL && (value & VA_SENS_START)) {
            g.sens_busy = 1;
            va_sim_sens.STAT &= ~VA_SENS_DONE;
        }
        return;
    }
    if (reg == &va_sim_clk.STAT) return; /* 唯讀 */
    if (reg == &va_sim_clk.SEL && value == 1u && !(va_sim_clk.STAT & VA_CLK_PLLRDY)) return;
    *reg = value;
}

void va_nvic_enable(int irqn)
{
    if (irqn == VA_UART1_IRQn) g.nvic_uart = 1;
    irq_eval();
}

uint32_t va_irq_save(void)
{
    uint32_t prev = (uint32_t)g.masked;
    g.masked = 1;
    return prev;
}

void va_irq_restore(uint32_t state)
{
    g.masked = (int)state;
    irq_eval(); /* 關中斷期間成立的條件，開中斷後立刻進入 */
}

const char *sim_platform(void)
{
    return "A：暫存器層級 UART、SysTick 中斷數毫秒、PLL 倍頻到 48 MHz";
}

void sim_reset(uint32_t time_origin_ms, host_sink_t sink, void *ctx)
{
    memset(&g, 0, sizeof g);
    memset(&va_sim_clk, 0, sizeof va_sim_clk);
    memset(&va_sim_systick, 0, sizeof va_sim_systick);
    memset(&va_sim_uart1, 0, sizeof va_sim_uart1);
    memset(&va_sim_sens, 0, sizeof va_sim_sens);
    va_sim_uart1.STAT = VA_UART_TXE; /* reset 後資料暫存器是空的 */
    g.sink = sink;
    g.sink_ctx = ctx;
    g.sensor_connected = 1;
    st.resets++;
    board_init(time_origin_ms);
    st.hclk_last = hclk();
    st.bauddiv_last = va_sim_uart1.BAUDDIV;
}

static void step(void)
{
    g.real_ms++;
    g.storm_now = 0;
    g_uart_irqs_this_ms = 0;
    irq_eval(); /* level 觸發：上一步留下的條件還成立 */

    /* 1. SysTick */
    if (va_sim_systick.CTRL & VA_SYSTICK_ENABLE) {
        uint64_t period = (uint64_t)va_sim_systick.LOAD + 1u;
        g.systick_cycles += hclk() / 1000u;
        while (g.systick_cycles >= period) {
            g.systick_cycles -= period;
            if (va_sim_systick.CTRL & VA_SYSTICK_TICKINT) {
                SysTick_Handler();
                st.systick_irqs++;
            }
        }
    }

    /* 2. 電腦 → 裝置 */
    g.rx_credit += LINE_BYTES_PER_S;
    while (g.to_dev_n && g.rx_credit >= 1000u) {
        uint8_t b = g.to_dev[g.to_dev_head];
        g.to_dev_head = (g.to_dev_head + 1u) % sizeof g.to_dev;
        g.to_dev_n--;
        g.rx_credit -= 1000u;
        if (!uart_on()) continue; /* UART 沒開：byte 就這樣消失 */
        if (baud_mismatch()) {
            b ^= 0x5Au;
            va_sim_uart1.STAT |= VA_UART_FE;
            st.framing++;
        }
        if (va_sim_uart1.STAT & VA_UART_RXNE) {
            va_sim_uart1.STAT |= VA_UART_ORE; /* 上一個還沒讀走：新的 byte 遺失 */
            st.overruns++;
        } else {
            g.rx_data = b;
            va_sim_uart1.STAT |= VA_UART_RXNE;
        }
        irq_eval();
    }
    if (!g.to_dev_n) g.rx_credit = 0;

    /* 3. 主迴圈；每圈之間感測器可能完成 */
    for (uint32_t k = 0; k < MAIN_LOOPS_PER_MS; k++) {
        if (g.real_ms >= g.block_until && !g.storm_now) board_main_once();
        if (g.sens_busy && g.sensor_connected) {
            g.sens_busy = 0;
            g.sens_n++;
            g.sens_value = (int16_t)(1000 + (int)(g.sens_n % 100u));
            va_sim_sens.STAT |= VA_SENS_DONE;
        }
    }

    /* 4. 裝置 → 電腦 */
    g.tx_credit += LINE_BYTES_PER_S;
    while (g.tx_full && g.tx_credit >= 1000u) {
        uint8_t b = g.tx_hold;
        if (baud_mismatch()) b ^= 0x5Au;
        g.tx_credit -= 1000u;
        g.tx_full = 0;
        va_sim_uart1.STAT |= VA_UART_TXE;
        if (g.sink) g.sink(g.sink_ctx, &b, 1u, g.real_ms);
        irq_eval();
    }
    if (!g.tx_full) g.tx_credit = 0;

    if (g.storm_now) st.storm_ms++;
    if (g_uart_irqs_this_ms > st.max_uart_irqs_per_ms) st.max_uart_irqs_per_ms = g_uart_irqs_this_ms;
}

void sim_run_ms(uint32_t ms)
{
    for (uint32_t i = 0; i < ms; i++) step();
}

uint32_t sim_now_ms(void) { return g.real_ms; }

void sim_host_send(const uint8_t *data, uint32_t n)
{
    for (uint32_t i = 0; i < n && g.to_dev_n < sizeof g.to_dev; i++)
        g.to_dev[(g.to_dev_head + g.to_dev_n++) % sizeof g.to_dev] = data[i];
}

void sim_block_main(uint32_t ms) { g.block_until = g.real_ms + ms + 1u; }

void sim_sensor_connect(int connected) { g.sensor_connected = connected; }

void sim_print_stats(void)
{
    printf("平台 A 統計（全部測試累計）：系統時脈 %u Hz、BAUDDIV %u\n", (unsigned)st.hclk_last,
           (unsigned)st.bauddiv_last);
    printf("  SysTick 中斷 %llu 次、UART 中斷 %llu 次（單一毫秒最多 %u 次）\n",
           (unsigned long long)st.systick_irqs, (unsigned long long)st.uart_irqs,
           (unsigned)st.max_uart_irqs_per_ms);
    printf("  overrun %u、framing error %u、中斷風暴 %u ms\n", (unsigned)st.overruns,
           (unsigned)st.framing, (unsigned)st.storm_ms);
}

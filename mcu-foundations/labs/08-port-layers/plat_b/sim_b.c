/*
 * sim_b.c：平台 B 的虛構 HAL、硬體模型與測試台。真實的板子上，HAL 由廠商提供，
 * 硬體模型與測試台不存在。
 *
 * 每 1 ms 真實時間推進一步：
 *   1. 電腦送來的 byte 依線路速率到達：放進 UART 資料暫存器、觸發中斷，由 HAL 的
 *      irq handler 放進使用者的緩衝區；沒有進行中的接收就記為 overrun。
 *   2. 主迴圈執行 MAIN_LOOPS_PER_MS 圈。每圈之後：HAL 若有傳送進行中而資料暫存器是空的，
 *      觸發 TX 中斷；感測器若有量測進行中而且接著，完成並觸發 I2C 中斷。
 *   3. UART 依線路速率把資料暫存器裡的 byte 送給電腦，每送完一個觸發一次 TX 中斷。
 * 低功耗 timer 依真實時間計數（每圈主迴圈 250 µs），起始值接近 0xFFFF，開機後很快就回繞。
 */
#include <stdio.h>
#include <string.h>

#include "sim.h"
#include "vendor_b.h"

#define HOST_BAUD 115200u
#define LINE_BYTES_PER_S 11520u
#define MAIN_LOOPS_PER_MS 4u
#define LPTIM_START 0xFFF0u

static struct {
    uint32_t real_ms, sub;
    host_sink_t sink;
    void *sink_ctx;
    uint8_t to_dev[4096];
    uint32_t to_dev_head, to_dev_n;
    uint32_t rx_credit, tx_credit, block_until;
    int sensor_connected;
    /* 晶片狀態 */
    int sys_ready, lptim_on, irq_uart, irq_i2c;
    vndb_uart_t *uart;     /* vndb_uart_init 登記的 handle */
    vndb_sensor_t *sensor; /* vndb_sensor_init 登記的 handle */
    int rx_full, tx_full;
    uint8_t rx_data, tx_data;
    int sens_pending, sens_ready;
    int16_t sens_value;
    uint32_t sens_n;
    uint16_t last_count;
} g;

/* 跨多次 sim_reset 累積的統計 */
static struct {
    uint64_t rx_cb, tx_cb, sensor_cb;
    uint32_t overruns, busy_returns, aborts, lptim_wraps, garbled;
} st;

/* ================= 虛構的廠商 HAL ================= */
void vndb_system_init(void) { g.sys_ready = 1; }

vndb_status_t vndb_uart_init(vndb_uart_t *h)
{
    if (!g.sys_ready || h->instance != 0u) return VNDB_ERROR;
    h->rx_busy = 0;
    h->tx_busy = 0;
    h->error_code = 0;
    g.uart = h;
    return VNDB_OK;
}

vndb_status_t vndb_uart_receive_it(vndb_uart_t *h, uint8_t *buf, uint16_t len)
{
    if (h != g.uart || !buf || !len) return VNDB_ERROR;
    if (h->rx_busy) return VNDB_BUSY;
    h->rx_buf = buf;
    h->rx_len = len;
    h->rx_count = 0;
    h->rx_busy = 1;
    return VNDB_OK;
}

vndb_status_t vndb_uart_transmit_it(vndb_uart_t *h, const uint8_t *buf, uint16_t len)
{
    if (h != g.uart || !buf || !len) return VNDB_ERROR;
    if (h->tx_busy) {
        st.busy_returns++;
        return VNDB_BUSY;
    }
    h->tx_buf = buf; /* 只記住指標：之後才一個一個 byte 從使用者的記憶體讀 */
    h->tx_len = len;
    h->tx_count = 0;
    h->tx_busy = 1;
    return VNDB_OK;
}

void vndb_uart_irq_handler(vndb_uart_t *h)
{
    if (g.rx_full) {
        g.rx_full = 0;
        if (h->rx_busy) {
            h->rx_buf[h->rx_count++] = g.rx_data;
            if (h->rx_count == h->rx_len) {
                h->rx_busy = 0; /* 先設回空閒再呼叫，callback 裡才能開始下一次 */
                st.rx_cb++;
                if (h->rx_done) h->rx_done(h);
            }
        } else {
            h->error_code |= VNDB_UART_ERR_OVERRUN; /* 沒有進行中的接收：這個 byte 遺失 */
            st.overruns++;
            if (h->error) h->error(h);
        }
    }
    if (!g.tx_full && h->tx_busy) {
        if (h->tx_count < h->tx_len) {
            g.tx_data = h->tx_buf[h->tx_count++];
            g.tx_full = 1;
        } else {
            h->tx_busy = 0;
            st.tx_cb++;
            if (h->tx_done) h->tx_done(h);
        }
    }
}

vndb_status_t vndb_lptim_start(vndb_lptim_t *h)
{
    if (!g.sys_ready || h->instance != 0u) return VNDB_ERROR;
    g.lptim_on = 1;
    return VNDB_OK;
}

uint16_t vndb_lptim_count(const vndb_lptim_t *h)
{
    (void)h;
    if (!g.lptim_on) return 0;
    uint64_t us = (uint64_t)g.real_ms * 1000u + (uint64_t)g.sub * (1000u / MAIN_LOOPS_PER_MS);
    return (uint16_t)(LPTIM_START + us * VNDB_LPTIM_HZ / 1000000u);
}

vndb_status_t vndb_sensor_init(vndb_sensor_t *h)
{
    if (!g.sys_ready) return VNDB_ERROR;
    h->busy = 0;
    g.sensor = h;
    return VNDB_OK;
}

vndb_status_t vndb_sensor_read_it(vndb_sensor_t *h)
{
    if (h != g.sensor) return VNDB_ERROR;
    if (h->busy) return VNDB_BUSY;
    h->busy = 1;
    g.sens_pending = 1;
    return VNDB_OK;
}

void vndb_sensor_abort(vndb_sensor_t *h)
{
    h->busy = 0;
    g.sens_pending = 0;
    g.sens_ready = 0;
    st.aborts++;
}

void vndb_sensor_irq_handler(vndb_sensor_t *h)
{
    if (!g.sens_ready) return;
    g.sens_ready = 0;
    if (!h->busy) return;
    h->busy = 0;
    st.sensor_cb++;
    if (h->done) h->done(h, g.sens_value);
}

void vndb_irq_enable(int irqn)
{
    if (irqn == VNDB_UART0_IRQn) g.irq_uart = 1;
    if (irqn == VNDB_I2C0_IRQn) g.irq_i2c = 1;
}

/* ================= 硬體模型與測試台 ================= */
static int baud_ok(void)
{
    if (!g.uart) return 0;
    uint32_t b = g.uart->baud;
    uint32_t diff = b > HOST_BAUD ? b - HOST_BAUD : HOST_BAUD - b;
    return diff * 100u <= HOST_BAUD * 3u;
}

static void uart_irq(void)
{
    if (g.irq_uart && g.uart) VNDB_UART0_IRQHandler();
}

/* 資料暫存器空、HAL 有傳送進行中：TX 中斷，直到暫存器又滿或傳送全部結束 */
static void service_tx(void)
{
    for (int guard = 0; guard < 4 && g.uart && !g.tx_full && g.uart->tx_busy; guard++) uart_irq();
}

static void sensor_progress(void)
{
    if (!g.sens_pending || !g.sensor_connected) return;
    g.sens_pending = 0;
    g.sens_ready = 1;
    g.sens_n++;
    g.sens_value = (int16_t)(1000 + (int)(g.sens_n % 100u));
    if (g.irq_i2c) VNDB_I2C0_IRQHandler();
}

const char *sim_platform(void)
{
    return "B：HAL callback 式 UART 與感測器、16 位元 32,768 Hz 自由計數 timer";
}

void sim_reset(uint32_t time_origin_ms, host_sink_t sink, void *ctx)
{
    memset(&g, 0, sizeof g);
    g.sink = sink;
    g.sink_ctx = ctx;
    g.sensor_connected = 1;
    g.last_count = LPTIM_START;
    board_init(time_origin_ms);
}

static void step(void)
{
    g.real_ms++;
    g.sub = 0;

    /* 1. 電腦 → 裝置 */
    g.rx_credit += LINE_BYTES_PER_S;
    while (g.to_dev_n && g.rx_credit >= 1000u) {
        uint8_t b = g.to_dev[g.to_dev_head];
        g.to_dev_head = (g.to_dev_head + 1u) % sizeof g.to_dev;
        g.to_dev_n--;
        g.rx_credit -= 1000u;
        if (!g.uart) continue;
        if (!baud_ok()) {
            b ^= 0x5Au;
            st.garbled++;
        }
        g.rx_data = b;
        g.rx_full = 1;
        uart_irq();
    }
    if (!g.to_dev_n) g.rx_credit = 0;

    /* 2. 主迴圈 */
    for (uint32_t k = 0; k < MAIN_LOOPS_PER_MS; k++) {
        g.sub = k;
        if (g.real_ms >= g.block_until) board_main_once();
        service_tx();
        sensor_progress();
    }

    /* 3. 裝置 → 電腦 */
    g.tx_credit += LINE_BYTES_PER_S;
    while (g.tx_full && g.tx_credit >= 1000u) {
        uint8_t b = baud_ok() ? g.tx_data : (uint8_t)(g.tx_data ^ 0x5Au);
        g.tx_credit -= 1000u;
        g.tx_full = 0;
        if (g.sink) g.sink(g.sink_ctx, &b, 1u, g.real_ms);
        service_tx();
    }
    if (!g.tx_full) g.tx_credit = 0;

    g.sub = 0;
    uint16_t c = vndb_lptim_count(NULL);
    if (c < g.last_count) st.lptim_wraps++;
    g.last_count = c;
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
    printf("平台 B 統計（全部測試累計）：低功耗 timer 回繞 %u 次\n", (unsigned)st.lptim_wraps);
    printf("  callback：接收 %llu 次、傳送完成 %llu 次、感測器 %llu 次\n", (unsigned long long)st.rx_cb,
           (unsigned long long)st.tx_cb, (unsigned long long)st.sensor_cb);
    printf("  overrun %u、VNDB_BUSY %u、感測器 abort %u、baud 不符的 byte %u\n", (unsigned)st.overruns,
           (unsigned)st.busy_returns, (unsigned)st.aborts, (unsigned)st.garbled);
}

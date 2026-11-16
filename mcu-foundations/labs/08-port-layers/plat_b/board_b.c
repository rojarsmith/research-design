/*
 * board_b.c：平台 B 的 BSP 與 driver。和 board_a.c 提供同樣的四個函式給應用核心，
 * 做法卻完全不同：時間要自己從 16 位元計數器延伸，UART 和感測器都是 HAL 的 callback。
 *
 * 上板時這個檔案搭配廠商 B 的 HAL 與啟動碼；main() 是：
 *   int main(void) { board_init(0); for (;;) board_main_once(); }
 */
#include <stdbool.h>
#include <string.h>

#include "platform.h"
#include "vendor_b.h"

/* ===== BSP：這塊板子的選擇 ===== */
#define UART_INSTANCE 0u /* 板載 debugger 的 USB 轉序列接在 UART0 */
#define UART_BAUD 115200u
#define SENSOR_BUS 0u
#define SENSOR_ADDR 0x48u

static vndb_uart_t s_uart;
static vndb_lptim_t s_lptim;
static vndb_sensor_t s_sensor;
static uint32_t s_uart_errors;

/* ===== driver：時間。16 位元、32,768 Hz 的計數器延伸成 32 位元毫秒 ===== */
static uint16_t s_last_count;
static uint32_t s_ms;
static uint32_t s_frac; /* 還不滿 1 ms 的部分，單位是 1/32768 ms */

static uint32_t b_now_ms(void)
{
    /* 只在主迴圈呼叫，所以這三個變數只有一個寫入者。
     * 兩次呼叫之間計數器最多只能回繞一次：主迴圈至少每 2 秒要呼叫一次。 */
    uint16_t c = vndb_lptim_count(&s_lptim);
    uint16_t delta = (uint16_t)(c - s_last_count);
    s_last_count = c;
    s_frac += (uint32_t)delta * 1000u; /* 最多 65535 × 1000，32 位元放得下 */
    s_ms += s_frac / VNDB_LPTIM_HZ;     /* 除以 2 的次方：沒有硬體除法的核心也只是位移 */
    s_frac %= VNDB_LPTIM_HZ;
    return s_ms;
}

/* ===== driver：UART 接收。每次收 1 byte，收到就在 callback 裡重新開始 ===== */
static uint8_t s_rx_byte;

static void on_rx_done(vndb_uart_t *h)
{
    app_uart_rx(s_rx_byte);
    vndb_uart_receive_it(h, &s_rx_byte, 1u); /* 立刻重新開始，否則下一個 byte 沒地方放 */
}

static void on_uart_error(vndb_uart_t *h)
{
    s_uart_errors++;
    h->error_code = 0;
    vndb_uart_receive_it(h, &s_rx_byte, 1u);
}

/* ===== driver：UART 傳送。一次最多 32 bytes，送完的 callback 裡接著送下一批 ===== */
static uint8_t s_tx_chunk[32]; /* 從 transmit_it 到 tx_done 之間，這塊記憶體屬於 HAL */
static volatile bool s_tx_busy;

/* 只在「沒有傳送進行中」時呼叫：主迴圈看到 s_tx_busy 為 false，或在 tx_done 裡 */
static void start_tx(void)
{
    uint32_t n = app_uart_tx(s_tx_chunk, sizeof s_tx_chunk);
    if (n == 0u) {
        s_tx_busy = false;
        return;
    }
    s_tx_busy = true;
    if (vndb_uart_transmit_it(&s_uart, s_tx_chunk, (uint16_t)n) != VNDB_OK)
        s_tx_busy = false; /* 不該發生；發生了代表 busy 的交接寫錯了，這 n 個 byte 已經遺失 */
}

static void on_tx_done(vndb_uart_t *h)
{
    (void)h;
    start_tx(); /* HAL 已經讀完 s_tx_chunk，現在可以重新填 */
}

static void b_tx_kick(void)
{
    if (!s_tx_busy) start_tx();
}

/* ===== driver：感測器。callback 只設旗標，判斷逾時是應用的事 ===== */
static volatile bool s_sensor_done;
static volatile int16_t s_sensor_value;

static void on_sensor_done(vndb_sensor_t *h, int16_t value)
{
    (void)h;
    s_sensor_value = value;
    s_sensor_done = true;
}

static void b_sensor_start(void)
{
    if (s_sensor.busy) vndb_sensor_abort(&s_sensor); /* 上一次逾時沒完成：收尾是 BSP 的責任 */
    s_sensor_done = false;
    (void)vndb_sensor_read_it(&s_sensor);
}

static int b_sensor_poll(int16_t *value)
{
    if (!s_sensor_done) return 0;
    *value = s_sensor_value;
    s_sensor_done = false;
    return 1;
}

/* ===== 中斷向量：名稱由啟動碼決定，內容交給 HAL ===== */
void VNDB_UART0_IRQHandler(void) { vndb_uart_irq_handler(&s_uart); }
void VNDB_I2C0_IRQHandler(void) { vndb_sensor_irq_handler(&s_sensor); }

/* ===== BSP：把平台接到應用 ===== */
static const platform_ops_t s_ops = {b_now_ms, b_tx_kick, b_sensor_start, b_sensor_poll};

void board_init(uint32_t time_origin_ms)
{
    /* 上板時 .bss 已由啟動碼清零；測試會多次呼叫 board_init，所以這裡全部重設 */
    memset(&s_uart, 0, sizeof s_uart);
    memset(&s_sensor, 0, sizeof s_sensor);
    s_tx_busy = false;
    s_sensor_done = false;
    s_uart_errors = 0;

    vndb_system_init();

    s_lptim.instance = 0u;
    vndb_lptim_start(&s_lptim);
    s_last_count = vndb_lptim_count(&s_lptim);
    s_ms = time_origin_ms;
    s_frac = 0;

    s_uart.instance = UART_INSTANCE;
    s_uart.baud = UART_BAUD;
    s_uart.rx_done = on_rx_done;
    s_uart.tx_done = on_tx_done;
    s_uart.error = on_uart_error;
    vndb_uart_init(&s_uart);

    s_sensor.bus = SENSOR_BUS;
    s_sensor.address = SENSOR_ADDR;
    s_sensor.done = on_sensor_done;
    vndb_sensor_init(&s_sensor);

    app_init(&s_ops);
    vndb_irq_enable(VNDB_UART0_IRQn);
    vndb_irq_enable(VNDB_I2C0_IRQn);
    vndb_uart_receive_it(&s_uart, &s_rx_byte, 1u);
}

void board_main_once(void) { app_poll(); }

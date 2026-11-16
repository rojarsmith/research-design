/*
 * app.c：應用層。把第五課的資料採集器（logger.c，一個字都沒改）接到 platform.h 的介面上。
 *
 * 這個檔案和 logger.c、frame.c、ring.c 合起來就是「應用核心」：
 * 只 include 自己的標頭與標準 C 標頭，任何一個平台的標頭都不需要。
 * 檢查方法：只給 core/ 與第五課目錄的 include 路徑，單獨編譯這幾個檔案。
 */
#include "logger.h"
#include "platform.h"

static logger_t s_logger;
static const platform_ops_t *s_ops;

/* 第五課的 sensor_port_t 帶一個 ctx 指標；平台介面不需要，這裡轉接 */
static void sensor_start(void *ctx)
{
    (void)ctx;
    s_ops->sensor_start();
}

static int sensor_poll(void *ctx, int16_t *value)
{
    (void)ctx;
    return s_ops->sensor_poll(value);
}

void app_init(const platform_ops_t *ops)
{
    s_ops = ops;
    sensor_port_t port = {sensor_start, sensor_poll, NULL};
    logger_init(&s_logger, port, ops->now_ms(), 0u);
}

void app_poll(void)
{
    logger_poll(&s_logger, s_ops->now_ms());
    /* 第五課的介面沒有「TX 有沒有資料」這個入口，板級程式只好自己去讀 logger 的欄位。
     * 移植時把它收進應用層：平台只會收到 tx_kick，不必知道 logger_t 長什麼樣子。 */
    if (ring_count(&s_logger.tx) != 0u) s_ops->tx_kick();
}

bool app_uart_rx(uint8_t byte) { return logger_uart_rx_isr(&s_logger, byte); }

uint32_t app_uart_tx(uint8_t *dst, uint32_t max)
{
    uint32_t n = 0;
    while (n < max && logger_uart_tx_isr(&s_logger, &dst[n])) n++;
    return n;
}

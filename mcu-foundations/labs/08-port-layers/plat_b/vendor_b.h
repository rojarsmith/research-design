/*
 * vendor_b.h：虛構廠商 B 的 HAL。風格接近常見的廠商 HAL：
 * 每個周邊一個 handle 結構、函式回傳狀態碼、非阻塞操作完成時在中斷裡呼叫使用者的 callback。
 *
 * 平台 B 的特徵（和平台 A 刻意不同）：
 *   時脈    vndb_system_init() 把時脈樹整個包起來，BSP 看不到 PLL 或除數；
 *           沒呼叫它，周邊初始化會回傳 VNDB_ERROR。
 *   時間    沒有 SysTick 式的毫秒中斷。只有一個 16 位元、32,768 Hz、自由計數的低功耗 timer，
 *           約每 2 秒回繞一次，而且不產生中斷。
 *   UART    非阻塞 API：
 *             vndb_uart_receive_it(h, buf, len)  收滿 len 個 byte 後呼叫 h->rx_done(h)。
 *               收完之後如果沒有再呼叫一次，下一個到達的 byte 沒地方放：記為 overrun 並呼叫 h->error(h)。
 *             vndb_uart_transmit_it(h, buf, len) 立即返回；HAL 一個一個 byte 從 buf 讀出去送，
 *               全部送完才呼叫 h->tx_done(h)。在那之前 buf 仍屬於 HAL，不能修改。
 *               上一次還沒送完就再呼叫，回傳 VNDB_BUSY。
 *           callback 在中斷裡執行，HAL 呼叫它之前已經把狀態設回空閒，所以可以在 callback 裡開始下一次。
 *   感測器  vndb_sensor_read_it(h) 立即返回，量測完成時在中斷裡呼叫 h->done(h, value)。
 *           感測器沒回應時 callback 永遠不會來；要重試必須先 vndb_sensor_abort(h)。
 *
 * 中斷向量的名稱由啟動碼決定（VNDB_UART0_IRQHandler、VNDB_I2C0_IRQHandler），
 * BSP 定義它們，並在裡面呼叫 HAL 的 irq handler。
 */
#ifndef VENDOR_B_H
#define VENDOR_B_H

#include <stdint.h>

typedef enum { VNDB_OK = 0, VNDB_ERROR = 1, VNDB_BUSY = 2 } vndb_status_t;

void vndb_system_init(void);

/* ---- UART ---- */
typedef struct vndb_uart vndb_uart_t;
typedef void (*vndb_uart_cb_t)(vndb_uart_t *h);

#define VNDB_UART_ERR_OVERRUN 0x01u

struct vndb_uart {
    /* 使用者填寫 */
    uint8_t instance;
    uint32_t baud;
    vndb_uart_cb_t rx_done;
    vndb_uart_cb_t tx_done;
    vndb_uart_cb_t error;
    void *user;
    /* HAL 內部使用 */
    uint8_t *rx_buf;
    uint16_t rx_len, rx_count;
    volatile uint8_t rx_busy;
    const uint8_t *tx_buf;
    uint16_t tx_len, tx_count;
    volatile uint8_t tx_busy;
    uint32_t error_code;
};

vndb_status_t vndb_uart_init(vndb_uart_t *h);
vndb_status_t vndb_uart_receive_it(vndb_uart_t *h, uint8_t *buf, uint16_t len);
vndb_status_t vndb_uart_transmit_it(vndb_uart_t *h, const uint8_t *buf, uint16_t len);
void vndb_uart_irq_handler(vndb_uart_t *h);

/* ---- 低功耗 timer ---- */
#define VNDB_LPTIM_HZ 32768u
typedef struct {
    uint8_t instance;
} vndb_lptim_t;

vndb_status_t vndb_lptim_start(vndb_lptim_t *h);
uint16_t vndb_lptim_count(const vndb_lptim_t *h);

/* ---- 感測器（接在 I2C 上） ---- */
typedef struct vndb_sensor vndb_sensor_t;
typedef void (*vndb_sensor_cb_t)(vndb_sensor_t *h, int16_t value);

struct vndb_sensor {
    uint8_t bus;
    uint8_t address;
    vndb_sensor_cb_t done;
    void *user;
    volatile uint8_t busy; /* HAL 寫；read_it 之後為 1，完成或 abort 後為 0 */
};

vndb_status_t vndb_sensor_init(vndb_sensor_t *h);
vndb_status_t vndb_sensor_read_it(vndb_sensor_t *h);
void vndb_sensor_abort(vndb_sensor_t *h);
void vndb_sensor_irq_handler(vndb_sensor_t *h);

/* ---- 中斷 ---- */
#define VNDB_UART0_IRQn 3
#define VNDB_I2C0_IRQn 7
void vndb_irq_enable(int irqn);

void VNDB_UART0_IRQHandler(void);
void VNDB_I2C0_IRQHandler(void);

#endif

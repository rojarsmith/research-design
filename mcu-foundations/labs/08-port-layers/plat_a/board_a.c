/*
 * board_a.c：平台 A 的 BSP 與 driver。應用核心不知道這個檔案的存在，
 * 它只拿到 platform_ops_t 裡的四個函式指標。
 *
 * 上板時這個檔案搭配廠商 A 的真實標頭檔與啟動碼；main() 是：
 *   int main(void) { board_init(0); for (;;) board_main_once(); }
 */
#include "platform.h"
#include "vendor_a.h"

/* ===== BSP：這塊板子的選擇 ===== */
#define HSI_HZ 8000000u   /* 內部振盪器 */
#define PLL_MUL 6u        /* 8 MHz × 6 = 48 MHz */
#define UART_BAUD 115200u /* 板載 debugger 的 USB 轉序列接在 UART1 */

static uint32_t s_core_hz; /* 實際的系統時脈：PLL 沒鎖定時退回內部振盪器 */
static volatile uint32_t s_ms;
static uint32_t s_uart_errors;

/* ===== driver：時脈 ===== */
static void clock_init(void)
{
    VA_WRITE(VA_CLK->PLLMUL, PLL_MUL);
    VA_WRITE(VA_CLK->CTRL, VA_CLK_PLLON);
    for (int i = 0; i < 1000 && !(VA_READ(VA_CLK->STAT) & VA_CLK_PLLRDY); i++) {
        /* 等 PLL 鎖定；任何等待都要有上限 */
    }
    if (VA_READ(VA_CLK->STAT) & VA_CLK_PLLRDY) {
        VA_WRITE(VA_CLK->SEL, 1u);
        s_core_hz = HSI_HZ * PLL_MUL;
    } else {
        s_core_hz = HSI_HZ; /* 沒鎖定就留在內部振盪器，後面的除數用這個頻率算 */
    }
    VA_WRITE(VA_CLK->GATE, VA_CLK_GATE_UART1 | VA_CLK_GATE_SENS);
}

/* ===== driver：時間。SysTick 每 1 ms 中斷一次，ISR 數毫秒 ===== */
void SysTick_Handler(void) { s_ms++; }

static uint32_t a_now_ms(void)
{
    return s_ms; /* 32 位元核心上，對齊的 32 位元讀取一次完成，不會讀到一半新一半舊 */
}

/* ===== driver：UART。一個 handler 處理收、送與錯誤 ===== */
void VA_UART1_IRQHandler(void)
{
    uint32_t st = VA_READ(VA_UART1->STAT);
    if (st & (VA_UART_ORE | VA_UART_FE)) {
        s_uart_errors++;
        VA_WRITE(VA_UART1->STAT, st & (VA_UART_ORE | VA_UART_FE)); /* 寫 1 清除 */
    }
    if (st & VA_UART_RXNE) app_uart_rx((uint8_t)VA_READ(VA_UART1->DATA)); /* 讀 DATA 清除 RXNE */
    if ((st & VA_UART_TXE) && (VA_READ(VA_UART1->CTRL) & VA_UART_TXIE)) {
        uint8_t b;
        if (app_uart_tx(&b, 1u))
            VA_WRITE(VA_UART1->DATA, b);
        else
            VA_WRITE(VA_UART1->CTRL, VA_READ(VA_UART1->CTRL) & ~VA_UART_TXIE); /* 沒資料就關掉，否則不停重入 */
    }
}

static void a_tx_kick(void)
{
    /* CTRL 的讀-改-寫會和 ISR 搶同一個暫存器，短暫關中斷 */
    uint32_t key = va_irq_save();
    VA_WRITE(VA_UART1->CTRL, VA_READ(VA_UART1->CTRL) | VA_UART_TXIE);
    va_irq_restore(key);
}

/* ===== driver：感測器。開始、輪詢旗標，都不等待 ===== */
static void a_sensor_start(void) { VA_WRITE(VA_SENS->CTRL, VA_SENS_START); }

static int a_sensor_poll(int16_t *value)
{
    if (!(VA_READ(VA_SENS->STAT) & VA_SENS_DONE)) return 0;
    *value = (int16_t)(uint16_t)VA_READ(VA_SENS->DATA);
    return 1;
}

/* ===== BSP：把平台接到應用 ===== */
static const platform_ops_t s_ops = {a_now_ms, a_tx_kick, a_sensor_start, a_sensor_poll};

void board_init(uint32_t time_origin_ms)
{
    s_uart_errors = 0; /* 上板時 .bss 已由啟動碼清零；測試會多次呼叫 board_init，所以這裡重設 */
    clock_init();
    s_ms = time_origin_ms;
    VA_WRITE(VA_SYSTICK->LOAD, s_core_hz / 1000u - 1u);
    VA_WRITE(VA_SYSTICK->VAL, 0u);
    VA_WRITE(VA_SYSTICK->CTRL, VA_SYSTICK_ENABLE | VA_SYSTICK_TICKINT);
    VA_WRITE(VA_UART1->BAUDDIV, (s_core_hz + UART_BAUD / 2u) / UART_BAUD);
    VA_WRITE(VA_UART1->CTRL, VA_UART_EN | VA_UART_RXIE);
    app_init(&s_ops);
    va_nvic_enable(VA_UART1_IRQn);
}

void board_main_once(void) { app_poll(); }

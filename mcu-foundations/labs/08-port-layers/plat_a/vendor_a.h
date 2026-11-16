/*
 * vendor_a.h：虛構廠商 A 的裝置標頭檔。風格接近 CMSIS 的裝置標頭：
 * 每個周邊是一個暫存器結構，位元用巨集命名，中斷 handler 的名稱由向量表決定。
 *
 * 平台 A 的特徵：
 *   時脈    內部 8 MHz 振盪器；PLL 倍頻後切換成系統時脈，周邊與核心同頻。
 *   時間    核心內的 SysTick：LOAD + 1 個時脈產生一次中斷，ISR 自己數 32 位元毫秒。
 *   UART    暫存器層級：一個資料暫存器、狀態旗標、一個中斷 handler 處理收與送。
 *           讀 DATA 清除 RXNE，寫 DATA 清除 TXE；ORE 與 FE 寫 1 清除。
 *           BAUDDIV = 周邊時脈 ÷ baud，由軟體自己算。
 *   感測器  暫存器層級：寫 START 開始，輪詢 DONE，讀 DATA 清除 DONE。
 *
 * 和真實標頭檔唯一的差別：真實晶片上 VA_READ/VA_WRITE 就是 volatile 存取，
 * 周邊在固定地址；這裡改成呼叫模擬器，讓模擬器看得到每一次讀寫並做出反應。
 * BSP 一律透過 VA_READ/VA_WRITE 存取暫存器，就像有些廠商標頭提供 READ_REG/WRITE_REG。
 */
#ifndef VENDOR_A_H
#define VENDOR_A_H

#include <stdint.h>

typedef struct {
    volatile uint32_t CTRL;   /* bit0 PLLON */
    volatile uint32_t STAT;   /* bit0 PLLRDY：PLL 鎖定後才成立 */
    volatile uint32_t PLLMUL; /* PLL 輸出 = 8 MHz × PLLMUL，有效值 2..8 */
    volatile uint32_t SEL;    /* 系統時脈：0 內部振盪器、1 PLL（PLL 未鎖定時切換無效） */
    volatile uint32_t GATE;   /* 周邊時脈閘控 */
} va_clk_t;
#define VA_CLK_PLLON (1u << 0)
#define VA_CLK_PLLRDY (1u << 0)
#define VA_CLK_GATE_UART1 (1u << 0)
#define VA_CLK_GATE_SENS (1u << 1)

typedef struct {
    volatile uint32_t CTRL; /* bit0 ENABLE、bit1 TICKINT */
    volatile uint32_t LOAD; /* 每 LOAD + 1 個系統時脈產生一次中斷 */
    volatile uint32_t VAL;
} va_systick_t;
#define VA_SYSTICK_ENABLE (1u << 0)
#define VA_SYSTICK_TICKINT (1u << 1)

typedef struct {
    volatile uint32_t CTRL;    /* bit0 EN、bit1 RXIE、bit2 TXIE */
    volatile uint32_t STAT;    /* bit0 RXNE、bit1 TXE、bit2 ORE、bit3 FE */
    volatile uint32_t DATA;
    volatile uint32_t BAUDDIV; /* 周邊時脈 ÷ baud，四捨五入 */
} va_uart_t;
#define VA_UART_EN (1u << 0)
#define VA_UART_RXIE (1u << 1)
#define VA_UART_TXIE (1u << 2)
#define VA_UART_RXNE (1u << 0)
#define VA_UART_TXE (1u << 1)
#define VA_UART_ORE (1u << 2)
#define VA_UART_FE (1u << 3)

typedef struct {
    volatile uint32_t CTRL; /* bit0 START：寫 1 開始一次量測 */
    volatile uint32_t STAT; /* bit0 DONE */
    volatile uint32_t DATA; /* 量測值（有號 16 位元）；讀取時清除 DONE */
} va_sens_t;
#define VA_SENS_START (1u << 0)
#define VA_SENS_DONE (1u << 0)

/* 周邊「地址」。真實標頭檔是 ((va_uart_t *)0x40011000u) 這類固定地址 */
extern va_clk_t va_sim_clk;
extern va_systick_t va_sim_systick;
extern va_uart_t va_sim_uart1;
extern va_sens_t va_sim_sens;
#define VA_CLK (&va_sim_clk)
#define VA_SYSTICK (&va_sim_systick)
#define VA_UART1 (&va_sim_uart1)
#define VA_SENS (&va_sim_sens)

uint32_t va_bus_read(volatile uint32_t *reg);
void va_bus_write(volatile uint32_t *reg, uint32_t value);
#define VA_READ(reg) va_bus_read(&(reg))
#define VA_WRITE(reg, value) va_bus_write(&(reg), (value))

/* 中斷控制 */
#define VA_UART1_IRQn 5
void va_nvic_enable(int irqn);
uint32_t va_irq_save(void);         /* 關中斷並回傳之前的狀態 */
void va_irq_restore(uint32_t state);

/* 中斷 handler：名稱由啟動碼的向量表決定，BSP 必須用這些名字定義 */
void SysTick_Handler(void);
void VA_UART1_IRQHandler(void);

#endif

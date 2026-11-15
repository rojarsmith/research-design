/*
 * ring.c：SPSC 環形緩衝區實作。
 *
 * 記憶體順序：
 *   生產者先寫資料，再以 release 發布新的 head；消費者以 acquire 讀 head，
 *   保證看到 head 前進時，對應的資料已經寫好。反方向的 tail 也一樣，
 *   保證生產者覆寫某格之前，消費者已經讀完那一格。
 *   單核心 MCU 上，ISR 與主迴圈之間主要靠它阻止編譯器重排；
 *   多核心或有 DMA 參與時，還需要對應的硬體屏障，這也是 acquire/release 的用途。
 *   這裡只用到原子的 load 與 store，不需要 read-modify-write 指令。
 *
 * 編譯時定義 RING_SIMULATE_PREEMPTION，會在每個「可能被中斷打斷」的位置
 * 呼叫 ring_sim_preempt()，讓桌機測試在那裡插入模擬的 ISR。上板時不要定義它。
 */
#include "ring.h"

#ifdef RING_SIMULATE_PREEMPTION
void ring_sim_preempt(void);
#define PREEMPT() ring_sim_preempt()
#else
#define PREEMPT() ((void)0)
#endif

bool ring_init(ring_t *r, uint8_t *storage, uint32_t capacity, uint32_t start_index)
{
    if (capacity < 2u || (capacity & (capacity - 1u)) != 0u || capacity > 0x80000000u)
        return false;
    r->buf = storage;
    r->mask = capacity - 1u;
    atomic_init(&r->head, start_index);
    atomic_init(&r->tail, start_index);
    atomic_init(&r->dropped, 0u);
    return true;
}

uint32_t ring_capacity(const ring_t *r) { return r->mask + 1u; }

uint32_t ring_count(ring_t *r)
{
    uint32_t tail = atomic_load_explicit(&r->tail, memory_order_acquire);
    uint32_t head = atomic_load_explicit(&r->head, memory_order_acquire);
    return head - tail; /* unsigned 減法：索引回繞後仍是正確的筆數 */
}

uint32_t ring_free(ring_t *r) { return ring_capacity(r) - ring_count(r); }

bool ring_push(ring_t *r, uint8_t byte)
{
    uint32_t head = atomic_load_explicit(&r->head, memory_order_relaxed); /* 自己的索引 */
    PREEMPT();
    uint32_t tail = atomic_load_explicit(&r->tail, memory_order_acquire);
    PREEMPT();
    if (head - tail >= ring_capacity(r)) {
        /* 滿了：丟掉最新的資料並計數。dropped 只有生產者寫，所以分開的 load/store 沒有競態 */
        uint32_t d = atomic_load_explicit(&r->dropped, memory_order_relaxed);
        atomic_store_explicit(&r->dropped, d + 1u, memory_order_relaxed);
        return false;
    }
    r->buf[head & r->mask] = byte;
    PREEMPT();
    atomic_store_explicit(&r->head, head + 1u, memory_order_release); /* 發布 */
    return true;
}

bool ring_write_all(ring_t *r, const uint8_t *src, uint32_t n)
{
    uint32_t head = atomic_load_explicit(&r->head, memory_order_relaxed);
    uint32_t tail = atomic_load_explicit(&r->tail, memory_order_acquire);
    PREEMPT();
    if (n > ring_capacity(r) - (head - tail)) {
        uint32_t d = atomic_load_explicit(&r->dropped, memory_order_relaxed);
        atomic_store_explicit(&r->dropped, d + n, memory_order_relaxed);
        return false; /* 不寫半個封包 */
    }
    for (uint32_t i = 0; i < n; i++) {
        r->buf[(head + i) & r->mask] = src[i];
        PREEMPT();
    }
    atomic_store_explicit(&r->head, head + n, memory_order_release); /* 整包一次發布 */
    return true;
}

bool ring_pop(ring_t *r, uint8_t *byte)
{
    uint32_t tail = atomic_load_explicit(&r->tail, memory_order_relaxed); /* 自己的索引 */
    PREEMPT();
    uint32_t head = atomic_load_explicit(&r->head, memory_order_acquire);
    PREEMPT();
    if (head == tail) return false;
    *byte = r->buf[tail & r->mask];
    PREEMPT();
    atomic_store_explicit(&r->tail, tail + 1u, memory_order_release); /* 歸還這一格 */
    return true;
}

uint32_t ring_read(ring_t *r, uint8_t *dst, uint32_t max)
{
    uint32_t n = 0;
    while (n < max && ring_pop(r, &dst[n])) n++;
    return n;
}

uint32_t ring_dropped(ring_t *r) { return atomic_load_explicit(&r->dropped, memory_order_relaxed); }

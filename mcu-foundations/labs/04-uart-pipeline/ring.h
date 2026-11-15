/*
 * ring.h：單一生產者、單一消費者（SPSC）的位元組環形緩衝區。
 *
 * 所有權規則（整個設計的重點）：
 *   head 只由生產者寫入，tail 只由消費者寫入；兩邊都只「讀」對方的索引。
 *   沒有任何變數被兩邊同時做讀-改-寫，所以不需要關中斷，也不需要 RMW 原子指令。
 *
 * 典型用法：
 *   UART RX：ISR 是生產者，主迴圈是消費者。
 *   UART TX：主迴圈是生產者，TX ISR 是消費者。
 *
 * 索引是 free-running 的 uint32_t，只在存取陣列時用 mask 取餘數；
 * head - tail 用 unsigned 減法算出目前筆數，回繞後仍正確（容量必須是 2 的次方）。
 */
#ifndef RING_H
#define RING_H

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t *buf;
    uint32_t mask;              /* 容量 - 1 */
    _Atomic uint32_t head;      /* 下一個要寫入的位置；只有生產者寫 */
    _Atomic uint32_t tail;      /* 下一個要讀出的位置；只有消費者寫 */
    _Atomic uint32_t dropped;   /* 因為滿了而丟掉的位元組數；只有生產者寫 */
} ring_t;

/* capacity 必須是 2 的次方；start_index 讓測試可以從接近回繞的位置開始 */
bool ring_init(ring_t *r, uint8_t *storage, uint32_t capacity, uint32_t start_index);

uint32_t ring_capacity(const ring_t *r);
uint32_t ring_count(ring_t *r);  /* 任一方呼叫都可以，結果是某個瞬間的快照 */
uint32_t ring_free(ring_t *r);

/* 生產者端 */
bool ring_push(ring_t *r, uint8_t byte);                     /* 滿了回傳 false 並計數 */
bool ring_write_all(ring_t *r, const uint8_t *src, uint32_t n); /* 全寫或全不寫 */

/* 消費者端 */
bool ring_pop(ring_t *r, uint8_t *byte);
uint32_t ring_read(ring_t *r, uint8_t *dst, uint32_t max);

uint32_t ring_dropped(ring_t *r);

#endif

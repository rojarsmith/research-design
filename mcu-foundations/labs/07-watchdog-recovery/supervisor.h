/*
 * supervisor.h：報到式的 watchdog 監督。決定「現在能不能餵狗」，不碰 watchdog 暫存器。
 *
 * 規則：
 *   每個受監督的活動在「證明自己有進度」的位置呼叫 sup_checkin()。
 *   有進度不等於有做事：沒有命令可處理的命令活動，只要確實檢查過接收緩衝區就算報到；
 *   有資料卻送不出去的上傳活動，即使函式每圈都被呼叫，也不能報到。
 *   主迴圈呼叫 sup_poll()；只有所有活動都在各自的期限內報到過，而且距離上次餵狗
 *   已經過 feed_every_ms，才回傳 true。呼叫端在回傳 true 時餵狗，這是唯一的餵狗點。
 *   一旦有活動逾期就鎖住，之後永遠回傳 false，讓 watchdog 把系統重啟到已知狀態。
 *
 * 只能在主迴圈呼叫。ISR 裡的活動不要直接報到，改成 ISR 增加計數、主迴圈檢查計數有沒有前進。
 */
#ifndef SUPERVISOR_H
#define SUPERVISOR_H

#include <stdbool.h>
#include <stdint.h>

#define SUP_MAX_TASKS 8
#define SUP_NONE (-1)

typedef struct {
    const char *name;
    uint32_t max_silence_ms; /* 超過這麼久沒報到，就判定這個活動停住 */
    uint32_t last_ms;
} sup_task_t;

typedef struct {
    sup_task_t task[SUP_MAX_TASKS];
    int count;
    uint32_t feed_every_ms; /* 用 window watchdog 時，這個間隔必須落在 window 之內 */
    uint32_t last_feed_ms;
    int stalled;            /* 第一個逾期的活動編號；SUP_NONE 表示都正常 */
} supervisor_t;

void sup_init(supervisor_t *s, uint32_t feed_every_ms, uint32_t now_ms);
int sup_add(supervisor_t *s, const char *name, uint32_t max_silence_ms, uint32_t now_ms);
void sup_set_deadline(supervisor_t *s, int id, uint32_t max_silence_ms);
void sup_checkin(supervisor_t *s, int id, uint32_t now_ms);
bool sup_poll(supervisor_t *s, uint32_t now_ms);

#endif

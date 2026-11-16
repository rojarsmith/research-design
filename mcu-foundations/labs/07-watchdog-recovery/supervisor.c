/*
 * supervisor.c：報到式監督的實作。時間一律用 unsigned 減法算經過時間，計數器回繞後仍正確。
 */
#include "supervisor.h"

void sup_init(supervisor_t *s, uint32_t feed_every_ms, uint32_t now_ms)
{
    s->count = 0;
    s->feed_every_ms = feed_every_ms;
    s->last_feed_ms = now_ms;
    s->stalled = SUP_NONE;
}

int sup_add(supervisor_t *s, const char *name, uint32_t max_silence_ms, uint32_t now_ms)
{
    if (s->count >= SUP_MAX_TASKS) return SUP_NONE;
    sup_task_t *t = &s->task[s->count];
    t->name = name;
    t->max_silence_ms = max_silence_ms;
    t->last_ms = now_ms;
    return s->count++;
}

void sup_set_deadline(supervisor_t *s, int id, uint32_t max_silence_ms)
{
    if (id >= 0 && id < s->count) s->task[id].max_silence_ms = max_silence_ms;
}

void sup_checkin(supervisor_t *s, int id, uint32_t now_ms)
{
    if (id >= 0 && id < s->count) s->task[id].last_ms = now_ms;
}

bool sup_poll(supervisor_t *s, uint32_t now_ms)
{
    if (s->stalled != SUP_NONE) return false; /* 已判定停住：不再餵，等 watchdog */

    for (int i = 0; i < s->count; i++) {
        if ((uint32_t)(now_ms - s->task[i].last_ms) > s->task[i].max_silence_ms) {
            s->stalled = i;
            return false;
        }
    }
    if ((uint32_t)(now_ms - s->last_feed_ms) < s->feed_every_ms) return false;
    s->last_feed_ms = now_ms;
    return true;
}

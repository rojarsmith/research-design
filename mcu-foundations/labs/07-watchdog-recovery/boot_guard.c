/*
 * boot_guard.c：開機紀錄與當機循環偵測。
 */
#include "boot_guard.h"

#define BOOT_MAGIC 0x42475244u

const char *reset_cause_name(reset_cause_t c)
{
    static const char *const names[] = {"未知", "上電", "reset 接腳", "watchdog", "window watchdog",
                                        "軟體"};
    return (unsigned)c < sizeof names / sizeof names[0] ? names[c] : "?";
}

/* 檢查碼只用來分辨「上次寫的」與「上電後的亂數」，不是安全機制 */
static uint32_t checksum(const boot_record_t *r)
{
    uint32_t h = 0x811C9DC5u;
    const uint32_t f[4] = {r->magic, r->boots, r->streak, r->stalled_task};
    for (int i = 0; i < 4; i++) {
        h ^= f[i];
        h *= 0x01000193u;
    }
    return h;
}

static void seal(boot_record_t *r)
{
    r->magic = BOOT_MAGIC;
    r->check = checksum(r);
}

void boot_guard_start(boot_record_t *rec, reset_cause_t cause, boot_report_t *out)
{
    bool valid = rec->magic == BOOT_MAGIC && rec->check == checksum(rec);
    if (cause == RESET_POWER_ON || !valid) {
        rec->boots = 0;
        rec->streak = 0;
        rec->stalled_task = BOOT_NO_TASK;
    }
    out->cause = cause;
    out->record_valid = valid;
    out->stalled_task = rec->stalled_task;
    rec->stalled_task = BOOT_NO_TASK; /* 讀過就清，下一次的紀錄才不會混到這一次 */

    if (cause == RESET_WATCHDOG || cause == RESET_WINDOW) rec->streak++;
    rec->boots++;
    seal(rec);

    out->boots = rec->boots;
    out->streak = rec->streak;
    out->safe_mode = rec->streak >= BOOT_SAFE_STREAK;
}

void boot_guard_note_stall(boot_record_t *rec, uint32_t task)
{
    rec->stalled_task = task;
    seal(rec);
}

void boot_guard_mark_stable(boot_record_t *rec)
{
    rec->streak = 0;
    seal(rec);
}

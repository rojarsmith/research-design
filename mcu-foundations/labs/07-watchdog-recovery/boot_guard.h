/*
 * boot_guard.h：開機時判斷「上一次是怎麼結束的」，偵測當機循環並決定是否進入安全模式。
 *
 * 輸入是已經解讀好的重啟原因。讀哪個暫存器、哪個位元代表什麼、怎麼清除，
 * 每顆晶片都不同，留給板級程式；這裡只處理跨平台的判斷規則。
 *
 * boot_record_t 要放在啟動碼不清零的 RAM 區（第二課的 .noinit）。warm reset 後
 * 內容通常還在，上電後是亂數，所以每次使用前都用 magic 與檢查碼確認可信。
 *
 * 規則：
 *   上電或紀錄不可信：從頭開始計數。
 *   watchdog 造成的重啟：連續當機次數 streak 加 1。
 *   streak 達到 BOOT_SAFE_STREAK：這次開機進入安全模式。
 *   穩定運作一段時間後呼叫 boot_guard_mark_stable()，streak 歸零。
 *   按 reset 鍵或軟體主動重啟不算當機，也不清除 streak。
 */
#ifndef BOOT_GUARD_H
#define BOOT_GUARD_H

#include <stdbool.h>
#include <stdint.h>

#define BOOT_SAFE_STREAK 3u
#define BOOT_NO_TASK 0xFFu

typedef enum {
    RESET_UNKNOWN,
    RESET_POWER_ON,
    RESET_PIN,      /* 外部 reset 接腳，例如按下 reset 鍵 */
    RESET_WATCHDOG, /* 獨立 watchdog */
    RESET_WINDOW,   /* window watchdog */
    RESET_SOFTWARE  /* 韌體主動要求，例如更新後重啟 */
} reset_cause_t;

typedef struct {
    uint32_t magic;
    uint32_t boots;        /* 上電以來的開機次數 */
    uint32_t streak;       /* 連續幾次在穩定之前就被 watchdog 重啟 */
    uint32_t stalled_task; /* supervisor 停止餵狗前留下的活動編號 */
    uint32_t check;        /* 前面欄位的檢查碼 */
} boot_record_t;

typedef struct {
    reset_cause_t cause;
    bool record_valid;
    uint32_t boots;
    uint32_t streak;
    uint32_t stalled_task; /* 上一次是哪個活動停住；BOOT_NO_TASK 表示沒有留下紀錄 */
    bool safe_mode;
} boot_report_t;

const char *reset_cause_name(reset_cause_t c);

void boot_guard_start(boot_record_t *rec, reset_cause_t cause, boot_report_t *out);
void boot_guard_note_stall(boot_record_t *rec, uint32_t task);
void boot_guard_mark_stable(boot_record_t *rec);

#endif

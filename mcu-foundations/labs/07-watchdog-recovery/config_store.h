/*
 * config_store.h：存在 Flash 上的設定。兩個槽輪流寫，任何一步斷電都不會失去上一份好設定。
 *
 * 這個模組只依賴下面的 flash_port_t，不碰任何暫存器，上板時把三個函式換成
 * 晶片的 Flash driver 即可。多數內建 NOR Flash 的規則如下，細節要查 reference manual：
 *   抹除以「頁」或「sector」為單位，抹除後每個 bit 都是 1（讀成 0xFF）；
 *   寫入只能把 1 變成 0，要把 0 變回 1 只能整頁抹除；
 *   寫入有最小單位（本模型是 4 bytes；有些晶片是 8 bytes 並帶 ECC，同一處抹除前只能寫一次）；
 *   抹寫次數有限，抹除一頁可能要數毫秒到數十毫秒。
 *
 * 一個槽的內容（little-endian，逐 byte 組裝，不直接寫 C struct）：
 *   位移  0  magic    u32  CONFIG_MAGIC
 *         4  version  u16  記錄的格式版本
 *         6  length   u16  payload 長度
 *         8  seq      u32  每寫一次加 1；兩個槽都有效時取較新的
 *        12  payload  依版本而定，補齊到 4 的倍數
 *         …  crc      u32  CRC-32，涵蓋前面所有內容；最後才寫
 *
 * 規則：
 *   永遠寫「目前沒有生效」的那個槽，生效中的那份在新的一份寫完並驗證之前不會被抹除。
 *   載入時兩個槽都檢查：magic、長度、CRC、版本、數值範圍；全部通過才算有效。
 *   兩個都有效取序號較新的；只有一個有效就用它；都無效就用預設值，並回報原因。
 */
#ifndef CONFIG_STORE_H
#define CONFIG_STORE_H

#include <stdbool.h>
#include <stdint.h>

#define CONFIG_MAGIC 0x53474643u /* "CFGS" */
#define CONFIG_VERSION 2u
#define CONFIG_HEADER_SIZE 12u
#define CONFIG_RECORD_MAX 32u    /* 一個槽最多用到幾 bytes */

/* 設定內容。v1 只有前兩個欄位，v2 新增 link_timeout_ms */
typedef struct {
    uint16_t period_ms;       /* 取樣週期 1..1000，預設 10 */
    uint8_t sensor_mode;      /* 0 一般、1 快速；預設 0 */
    uint16_t link_timeout_ms; /* 多久沒收到電腦的命令就標示斷線；0 = 不檢查，否則 100..60000；預設 3000 */
} config_t;

/* 板級程式提供的 Flash 存取。page 是槽的編號 0 或 1，offset 與 len 都是 4 的倍數 */
typedef struct {
    bool (*erase)(void *ctx, uint32_t page);
    bool (*program)(void *ctx, uint32_t page, uint32_t offset, const uint8_t *src, uint32_t len);
    void (*read)(void *ctx, uint32_t page, uint32_t offset, uint8_t *dst, uint32_t len);
    void *ctx;
} flash_port_t;

typedef enum {
    SLOT_EMPTY,      /* 抹除後沒寫過 */
    SLOT_BAD_CRC,    /* magic 不對或 CRC 不符：寫到一半斷電、位元翻轉 */
    SLOT_BAD_FORMAT, /* 版本不認得，或長度和版本不一致 */
    SLOT_BAD_VALUE,  /* 格式正確，但數值超出範圍 */
    SLOT_OK
} slot_state_t;

typedef struct {
    flash_port_t flash;
    int active;   /* 目前生效的槽：0、1；-1 表示正在用預設值 */
    uint32_t seq; /* 生效那一槽的序號；用預設值時為 0 */
} config_store_t;

typedef struct {
    int source;            /* 0、1 是槽，-1 是預設值 */
    slot_state_t slot[2];  /* 每個槽的判定 */
    uint16_t version[2];   /* 每個槽讀到的版本（無法判讀時為 0） */
    uint32_t seq;          /* 生效那一份的序號 */
    bool upgraded;         /* 生效的設定是從舊版格式轉換來的，還沒寫回新格式 */
} config_report_t;

uint32_t crc32_ieee(const uint8_t *data, uint32_t n);
const char *slot_state_name(slot_state_t s);

void config_defaults(config_t *c);
bool config_valid(const config_t *c);
bool config_equal(const config_t *a, const config_t *b);

/* 把設定編成目前版本的槽內容，回傳總長度（含 CRC） */
uint32_t config_encode(const config_t *c, uint32_t seq, uint8_t *out);

void config_store_init(config_store_t *st, flash_port_t flash);
void config_load(config_store_t *st, config_t *out, config_report_t *rep);
/* 無效的設定不會寫入；回傳 false 表示這次沒有寫成，生效中的設定不變 */
bool config_save(config_store_t *st, const config_t *c);

#endif

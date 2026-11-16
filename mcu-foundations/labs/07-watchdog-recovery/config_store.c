/*
 * config_store.c：雙槽設定儲存。純邏輯，只透過 flash_port_t 存取 Flash。
 */
#include "config_store.h"

#include <string.h>

#define PAYLOAD_V1 4u /* period u16、sensor_mode u8、保留 u8 */
#define PAYLOAD_V2 6u /* v1 的內容，再加 link_timeout_ms u16 */
#define PAYLOAD_MAX (CONFIG_RECORD_MAX - CONFIG_HEADER_SIZE - 4u)

static void put_u16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
}
static void put_u32(uint8_t *p, uint32_t v)
{
    for (int i = 0; i < 4; i++) p[i] = (uint8_t)(v >> (8 * i));
}
static uint16_t get_u16(const uint8_t *p) { return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8)); }
static uint32_t get_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static uint32_t pad4(uint32_t n) { return (n + 3u) & ~3u; }

uint32_t crc32_ieee(const uint8_t *data, uint32_t n)
{
    uint32_t crc = 0xFFFFFFFFu;
    for (uint32_t i = 0; i < n; i++) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; bit++) crc = (crc & 1u) ? (crc >> 1) ^ 0xEDB88320u : crc >> 1;
    }
    return ~crc;
}

const char *slot_state_name(slot_state_t s)
{
    static const char *const names[] = {"空白", "CRC 錯", "格式錯", "數值錯", "有效"};
    return (unsigned)s < sizeof names / sizeof names[0] ? names[s] : "?";
}

void config_defaults(config_t *c)
{
    c->period_ms = 10u;
    c->sensor_mode = 0u;
    c->link_timeout_ms = 3000u;
}

bool config_valid(const config_t *c)
{
    if (c->period_ms < 1u || c->period_ms > 1000u) return false;
    if (c->sensor_mode > 1u) return false;
    if (c->link_timeout_ms != 0u && (c->link_timeout_ms < 100u || c->link_timeout_ms > 60000u))
        return false;
    return true;
}

bool config_equal(const config_t *a, const config_t *b)
{
    return a->period_ms == b->period_ms && a->sensor_mode == b->sensor_mode &&
           a->link_timeout_ms == b->link_timeout_ms;
}

uint32_t config_encode(const config_t *c, uint32_t seq, uint8_t *out)
{
    uint32_t body = CONFIG_HEADER_SIZE + pad4(PAYLOAD_V2);
    memset(out, 0, body);
    put_u32(&out[0], CONFIG_MAGIC);
    put_u16(&out[4], CONFIG_VERSION);
    put_u16(&out[6], PAYLOAD_V2);
    put_u32(&out[8], seq);
    put_u16(&out[12], c->period_ms);
    out[14] = c->sensor_mode;
    put_u16(&out[16], c->link_timeout_ms);
    put_u32(&out[body], crc32_ieee(out, body));
    return body + 4u;
}

void config_store_init(config_store_t *st, flash_port_t flash)
{
    st->flash = flash;
    st->active = -1;
    st->seq = 0;
}

/* 序號比較用模數差：a 比 b 新，代表 a - b 落在 1..2^31-1。回繞後仍正確 */
static bool seq_newer(uint32_t a, uint32_t b)
{
    uint32_t d = a - b;
    return d != 0u && d < 0x80000000u;
}

static slot_state_t check_slot(config_store_t *st, uint32_t page, config_t *out, uint32_t *seq,
                               uint16_t *version)
{
    uint8_t rec[CONFIG_RECORD_MAX];
    *version = 0;
    st->flash.read(st->flash.ctx, page, 0, rec, CONFIG_HEADER_SIZE);

    uint32_t magic = get_u32(&rec[0]);
    if (magic != CONFIG_MAGIC) {
        for (uint32_t i = 0; i < CONFIG_HEADER_SIZE; i++)
            if (rec[i] != 0xFFu) return SLOT_BAD_CRC; /* 有內容卻不是設定：寫一半或損壞 */
        return SLOT_EMPTY;
    }
    uint16_t ver = get_u16(&rec[4]);
    uint16_t len = get_u16(&rec[6]);
    if (len > PAYLOAD_MAX) return SLOT_BAD_FORMAT; /* 長度離譜，連 CRC 在哪都不知道 */

    uint32_t body = CONFIG_HEADER_SIZE + pad4(len);
    st->flash.read(st->flash.ctx, page, CONFIG_HEADER_SIZE, &rec[CONFIG_HEADER_SIZE],
                   pad4(len) + 4u);
    if (crc32_ieee(rec, body) != get_u32(&rec[body])) return SLOT_BAD_CRC;

    *version = ver;
    *seq = get_u32(&rec[8]);
    const uint8_t *p = &rec[CONFIG_HEADER_SIZE];
    if (ver == 1u && len == PAYLOAD_V1) {
        out->period_ms = get_u16(&p[0]);
        out->sensor_mode = p[2];
        out->link_timeout_ms = 3000u; /* 升級：舊版沒有這個欄位，填入新欄位的預設值 */
    } else if (ver == 2u && len == PAYLOAD_V2) {
        out->period_ms = get_u16(&p[0]);
        out->sensor_mode = p[2];
        out->link_timeout_ms = get_u16(&p[4]);
    } else {
        return SLOT_BAD_FORMAT; /* 例如較新的韌體寫的 v3：看不懂就不要猜 */
    }
    return config_valid(out) ? SLOT_OK : SLOT_BAD_VALUE;
}

void config_load(config_store_t *st, config_t *out, config_report_t *rep)
{
    config_t c[2];
    uint32_t seq[2] = {0u, 0u};
    memset(rep, 0, sizeof *rep);
    for (uint32_t i = 0; i < 2u; i++) rep->slot[i] = check_slot(st, i, &c[i], &seq[i], &rep->version[i]);

    int pick = -1;
    if (rep->slot[0] == SLOT_OK && rep->slot[1] == SLOT_OK)
        pick = seq_newer(seq[1], seq[0]) ? 1 : 0;
    else if (rep->slot[0] == SLOT_OK)
        pick = 0;
    else if (rep->slot[1] == SLOT_OK)
        pick = 1;

    if (pick < 0) {
        config_defaults(out);
        st->active = -1;
        st->seq = 0;
        rep->source = -1;
        return;
    }
    *out = c[pick];
    st->active = pick;
    st->seq = seq[pick];
    rep->source = pick;
    rep->seq = seq[pick];
    rep->upgraded = rep->version[pick] < CONFIG_VERSION;
}

bool config_save(config_store_t *st, const config_t *c)
{
    if (!config_valid(c)) return false; /* 無效設定不寫入，生效中的那份不受影響 */

    uint32_t target = st->active == 0 ? 1u : 0u; /* 永遠寫沒有生效的那一槽 */
    uint32_t seq = st->seq + 1u;
    uint8_t rec[CONFIG_RECORD_MAX];
    uint32_t n = config_encode(c, seq, rec);

    /* 順序：抹除 → 寫入 CRC 以外的內容 → 最後寫 CRC。
     * 任何一步斷電，這一槽都通不過 CRC，另一槽的舊設定完好如初。 */
    if (!st->flash.erase(st->flash.ctx, target)) return false;
    if (!st->flash.program(st->flash.ctx, target, 0u, rec, n - 4u)) return false;
    if (!st->flash.program(st->flash.ctx, target, n - 4u, &rec[n - 4u], 4u)) return false;

    uint8_t back[CONFIG_RECORD_MAX];
    st->flash.read(st->flash.ctx, target, 0u, back, n);
    if (memcmp(back, rec, n) != 0) return false; /* 讀回比對：寫入錯誤或沒有先抹除 */

    st->active = (int)target;
    st->seq = seq;
    return true;
}

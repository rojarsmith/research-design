/*
 * frame.c：CRC、封包編碼與逐位元組解析器。純邏輯，不碰任何硬體，
 * 所以同一份程式可以在桌機測試、在 MCU 上解析命令、在電腦端解析資料。
 */
#include "frame.h"

#include <string.h>

uint16_t crc16_ccitt_false(const uint8_t *data, uint32_t n)
{
    uint16_t crc = 0xFFFFu;
    for (uint32_t i = 0; i < n; i++) {
        crc ^= (uint16_t)(data[i] << 8);
        for (int bit = 0; bit < 8; bit++)
            crc = (crc & 0x8000u) ? (uint16_t)((crc << 1) ^ 0x1021u) : (uint16_t)(crc << 1);
    }
    return crc;
}

uint32_t frame_encode(uint8_t type, const uint8_t *payload, uint8_t len, uint8_t *out)
{
    if (len > FRAME_MAX_PAYLOAD) return 0;
    out[0] = FRAME_SYNC;
    out[1] = len;
    out[2] = type;
    if (len) memcpy(&out[3], payload, len);
    put_u16(&out[3u + len], crc16_ccitt_false(&out[1], 2u + len));
    return FRAME_OVERHEAD + len;
}

enum { PS_HUNT, PS_LEN, PS_TYPE, PS_PAYLOAD, PS_CRC_LO, PS_CRC_HI };

void parser_init(parser_t *p, uint32_t timeout_ms, frame_handler_t on_frame, void *ctx)
{
    memset(p, 0, sizeof *p);
    p->state = PS_HUNT;
    p->timeout_ms = timeout_ms;
    p->on_frame = on_frame;
    p->ctx = ctx;
}

uint32_t parser_errors(const parser_t *p) { return p->err_crc + p->err_len + p->err_timeout; }

static void reset(parser_t *p)
{
    p->state = PS_HUNT;
    p->n = 0;
}

/* 出錯時，raw[0] 那個 SYNC 是假的；把 raw[1..n) 放回待掃描佇列最前面重新找。
 * 這樣即使假 SYNC 吞掉了後面一個完整封包，也還能把它找回來。 */
static void resync(parser_t *p)
{
    uint32_t keep = p->n - 1u;
    memmove(p->replay + keep, p->replay, p->replay_n);
    memcpy(p->replay, p->raw + 1, keep);
    p->replay_n += keep;
    p->bytes_skipped++;
    reset(p);
}

static void step(parser_t *p, uint8_t b)
{
    if (p->state == PS_HUNT) {
        if (b == FRAME_SYNC) {
            p->raw[0] = b;
            p->n = 1;
            p->state = PS_LEN;
        } else {
            p->bytes_skipped++;
        }
        return;
    }

    p->raw[p->n++] = b;
    switch (p->state) {
    case PS_LEN:
        if (b > FRAME_MAX_PAYLOAD) { /* 過長：不可能是合法封包 */
            p->err_len++;
            resync(p);
            return;
        }
        p->state = PS_TYPE;
        break;
    case PS_TYPE:
        p->state = p->raw[1] ? PS_PAYLOAD : PS_CRC_LO;
        break;
    case PS_PAYLOAD:
        if (p->n == 3u + p->raw[1]) p->state = PS_CRC_LO;
        break;
    case PS_CRC_LO:
        p->state = PS_CRC_HI;
        break;
    default: { /* PS_CRC_HI：整個封包收齊 */
        uint32_t body = 2u + p->raw[1];
        uint16_t calc = crc16_ccitt_false(&p->raw[1], body);
        if (calc != get_u16(&p->raw[1u + body])) {
            p->err_crc++;
            resync(p);
            return;
        }
        frame_t f;
        f.len = p->raw[1];
        f.type = p->raw[2];
        memcpy(f.payload, &p->raw[3], f.len);
        p->frames_ok++;
        reset(p);
        if (p->on_frame) p->on_frame(p->ctx, &f);
        return;
    }
    }
}

void parser_feed(parser_t *p, uint8_t byte, uint32_t now_ms)
{
    /* 封包收到一半就停住：可能是斷線、傳送端重啟或位元組遺失。放棄這半個封包 */
    if (p->state != PS_HUNT && (uint32_t)(now_ms - p->last_byte_ms) > p->timeout_ms) {
        p->err_timeout++;
        p->bytes_skipped += p->n;
        reset(p);
    }
    p->last_byte_ms = now_ms;

    p->replay[p->replay_n++] = byte;
    while (p->replay_n) {
        uint8_t b = p->replay[0];
        p->replay_n--;
        memmove(p->replay, p->replay + 1, p->replay_n);
        step(p, b);
    }
}
